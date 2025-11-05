#pragma once

class CFrame;
class CFrameSet;

namespace SpriteSheet
{
    enum Flag
    {
        noflag = 0,
        sorted = 1,
    };
};

bool toSpriteSheet(CFrame &sheet, CFrameSet &set, const SpriteSheet::Flag flags);
