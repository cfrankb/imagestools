# Custom grid size — implementation plan

Status: implemented (all 4 changes). `HitboxEditor` builds clean against Qt 6.11.
The paint/clamp check ran as a throwaway (`GridScene::setMajorGrid(0,0)` then
`render()` is SIGFPE without change 2, passes with it); it was not committed,
since wiring a second target into both CMakeLists and hitbox.pro for one GUI
smoke test is not worth the build churn. Steps 1-4 below still need a click-through.
Scope: `hitbox_editor` only. JSON format unchanged.

## Problem

The major-grid (frame) size can only be one of a fixed list:

```cpp
// hitbox.cpp:9
const QStringList sizes = {"16", "24", "32", "48", "64", "128", "256"};
```

Sheets whose frames are not in that list cannot be expressed. Real case:

```
out/arachine/ARACHINE_ATTACK.png    1560x96   → 15 × 104
out/arachine/ARACHINE_DAMAGED.png   1248x96   → 12 × 104
out/arachine/ARACHINE_IDLE.png       312x96   →  3 × 104
```

104 and 96 are both off-list. The committed `ARACHINE_ATTACK.json` shows what the
tool recorded instead of 104x96:

```json
"frame": { "cols": 24, "height": 64, "width": 64 }   // 1560 / 64 = 24.375 → truncated
```

`cols` is derived from the chosen width (`editor.cpp:286`), so an inexact width
yields a wrong `cols`. The consumer `bin/hitbox.py:52-56` uses `cols` as
`frames_per_row` to map a hitbox to a frame id, so every hitbox on this sheet is
assigned to the wrong frame downstream. This is the actual damage.

Three related defects found while reading the code:

1. **Silent mismatch on load.** `reloadData` calls `setMajorGrid(w,h)` with the
   saved value, then looks the value up in `sizes` to set the combo index
   (`editor.cpp:412-422`). Any value outside the list leaves the combo showing a
   size that is not the one in effect.
2. **Div-by-zero / paint hang.** `GridScene::drawBackground` does
   `% majorGridH` and `x += majorGridH` (`hitbox.cpp:59-66`). `reloadData`
   passes `frame["width"].toInt()` unguarded, so a JSON with `"frame": {}` or a
   missing/zero width loads 0 and hangs the paint loop. Already reachable today.
3. **Silent bad save.** Nothing checks that the image divides evenly by the
   frame size before `cols` is written.

## Goal

Odd frame sizes (104x96) must be enterable and survive a save/load round trip,
without making the ordinary 16/32/64 case any slower than one dropdown click.

## Design

Keep the combo boxes as preset pickers and make them editable. The dropdown
stays the easy path; typing is the escape hatch. No new widget, no dialog, no
preference, no mode switch.

## Changes

### 1. `editor.cpp:94-103` — editable combos

```cpp
gridSizeComboH->setEditable(true);
gridSizeComboH->setInsertPolicy(QComboBox::NoInsert);
gridSizeComboH->setValidator(new QIntValidator(1, 4096, gridSizeComboH));
```

Repeat for `gridSizeComboV`. `NoInsert` is required: the default
`InsertAtBottom` permanently appends every typed value to the preset list, which
is how one-off sizes leak into the "normal" list. The validator rejects 0 and
negatives at the line edit.

No change needed to the existing `currentTextChanged` lambdas
(`editor.cpp:112-124`) — they already forward `currentText().toInt()` to
`setMajorGrid`.

### 2. `hitbox.h:23` — clamp at the choke point

```cpp
void setMajorGrid(int h, int v) { if (h > 0 && v > 0) { majorGridH = h; majorGridV = v; } update(); }
```

Every caller routes through here (both combos and `reloadData`), so one guard
covers hand-written JSON too and closes defect 2. A validator alone would not.

### 3. `editor.cpp:413-422` — restore any value

Delete the `for (auto const &size: sizes)` lookup loop and set the text directly:

```cpp
gridSizeComboH->setCurrentText(QString::number(w));
gridSizeComboV->setCurrentText(QString::number(h));
```

Closes defect 1. Net line count goes down.

### 4. `editor.cpp:281-286` — divisibility warning on save

```cpp
if (currentPixmap.width() % frameWidth || currentPixmap.height() % frameHeight)
    statusBar()->showMessage(QString("Warning: %1x%2 not divisible by frame %3x%4")
        .arg(currentPixmap.width()).arg(currentPixmap.height())
        .arg(frameWidth).arg(frameHeight), 8000);
```

Warn, do not block: some sheets are padded and the user may know better. Closes
defect 3, and would have caught the arachine sheet at save time.

## Explicitly out of scope

- **Snapping.** The `GRID = 8` constant (`hitbox.h:10`) drives drag snap,
  `snapToGrid` and arrow-key step. It is independent of the major grid and stays
  as is. Making snap follow the frame size is a separate change: `GRID` becomes
  instance state threaded into the static `HitboxItem::snap`, `snapToGrid` and
  `keyPressEvent` (~30-50 lines), plus a decision on whether to persist it.
- **Minor grid.** Hardcoded `8` in `hitbox.cpp:36`; never user-settable, so
  nothing to save or restore.
- Adding 96/104 to `sizes`. One-off sheet; the list is for the common case.
- Linking H to V so editing one follows the other. Surprising when H != V, which
  is exactly the 104x96 case.
- Rewriting the existing wrong `ARACHINE_*.json` files. After this change they
  load as 64x64 (their recorded value); re-enter 104x96 and save to correct them.
  Scripting that fix is a separate task if more than the five arachine sheets
  need it.

## Follow-ups now implemented

- `checkGridFits()` (`editor.cpp`): the divisibility warning is no longer save-only.
  It fires whenever the grid or the image changes — grid combo edits
  (`applyGridSize`), every JSON load (`reloadData`), `onOpen` with no sibling JSON,
  and Save. The duplicated grid lambdas collapsed into one `applyGridSize` slot.
- `roundGrid()` / `commitGridSizes()`: a grid size is only usable as a positive
  multiple of 8 (the minor grid and the hitbox snap are both 8px). Typed and
  JSON-loaded values are rounded to the nearest multiple of 8 (ties up, anything
  under 8 becomes 8) when editing ends, the field is rewritten with the rounded
  value and the status bar says so. Half-typed values are never applied as final
  because rounding happens on `editingFinished`, not on each keystroke; a blank
  field is still safe mid-typing because `GridScene::setMajorGrid` ignores `<= 0`.
  `rememberGridSize` refuses non-multiples, and `validSizes()` filters legacy
  `customGridW` / `customGridH` entries on startup. `onSave` commits first, so the
  `width/height/cols` it writes always satisfy the rule (its old "bad grid size"
  dialog became unreachable and was removed).

- `rememberGridSize()`: the last 4 typed sizes per axis (`kMaxCustomGrid`) are kept
  most-recent-first at the top of each dropdown and in QSettings under
  `customGridW` / `customGridH`. Presets are never displaced and never re-added as
  customs. Sizes arriving from a JSON are remembered too, so an imported 104x96 is
  one click next time.

## Verification

Build with the existing CMake target (`CMakeLists.txt`, Qt 6.11 widgets).

Manual, in this order:

1. Open `out/arachine/ARACHINE_ATTACK.png`. Sets grid to 104 H / 96 V. Confirm
   the vertical lines land on frame boundaries and the status bar shows no
   divisibility warning.
2. Save; confirm `ARACHINE_ATTACK.json` reads `"width": 104, "height": 96,
   "cols": 15`.
3. Reload the same file. The two combos must display `104` and `96` — this is
   the behaviour that is broken today.
4. Type `64` in both; confirm one dropdown click still reaches 32/48/64 and the
   preset list was not extended by the typed `104`.
5. Type `0` or clear the field. Grid must keep its previous size, no freeze.
6. Load a hand-written JSON containing `"frame": {}`. Must not hang; grid falls
   back to the previous size.
