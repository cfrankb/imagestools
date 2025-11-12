import os
from PIL import Image
import pathlib
import copy


def example():
    # Open the two images
    image1 = Image.open("background.png")
    image2 = Image.open("foreground.png")

    # Ensure images have the same mode and size (e.g., RGBA)
    image1 = image1.convert("RGBA")
    image2 = image2.convert("RGBA")

    # Blend the images with an alpha factor (e.g., 0.5 for 50% blend)
    blended_image = Image.blend(image1, image2, alpha=0.5)

    # Save or display the result
    blended_image.save("blended_image.png")
    blended_image.show()


def turn_image_white(img):
    datas = img.getdata()
    newData = []
    print(".... *white")

    for item in datas:
        # item is a tuple (R, G, B, A)
        if item[3] > 0:  # If the pixel is not fully transparent (alpha > 0)
            newData.append((255, 255, 255, 255))
        else:
            newData.append(item)  # Keep fully transparent pixels as they are

    img.putdata(newData)
    return img


def process_strip(
    monster, image_path, layers, basename, dest_path, strip_height, frame_width
):
    print(image_path)
    img = None

    for layer in layers:
        part_path = image_path.replace("[LAYER]", layer)
        if not os.path.isfile(part_path):
            continue
        print(f".... {layer}")
        if img:
            img_part = Image.open(part_path)
            if layer == "red":
                img_part = turn_image_white(img_part)
            img = Image.alpha_composite(img, img_part)
        else:
            img = Image.open(part_path)
            width, height = img.size
            # print(f"w:{width} h:{height}")

    # print(width/8)
    strip_count = int(height / strip_height)
    strip_order = [1, 0, 2, 3]

    # print(f"frame: {width/frame_width}")
    frames = int(width / frame_width)
    files = []

    strips = []

    # split into strips
    for i in range(strip_count):
        y = i * strip_height
        box = [0, y, width, y + strip_height]
        # print(i, box)
        strip = img.crop(box)
        strips.append(strip)

    new_image = Image.new("RGBA", (width, height))
    for i in range(strip_count):
        j = strip_order[i]
        new_image.paste(im=strips[j], box=(0, i * strip_height))
    out_path = f"{dest_path}{basename}.png"
    print(out_path)
    new_image.save(out_path)
    files.append(
        f"{monster.lower()}/{os.path.basename(out_path)}:{frame_width},{strip_height}"
    )

    return files, frames


monsters: list[dict] = [
    {
        "name": "Plant1",
        "layers": ["body", "brown", "head", "red", "swing"],
        "skip": False,
    },
    {
        "name": "Beholder1",
        "layers": ["body", "fire", "red", "swing"],
        "skip": True,
    },
    {
        "name": "Beholder3",
        "layers": ["body", "fire", "red", "swing"],
        "skip": True,
    },
    {
        "name": "Slime3",
        "layers": ["body"],
        "skip": True,
    },
]


for monster in monsters:
    if monster["skip"]:
        continue
    print(monster)
    monster_name = monster["name"]
    layers = monster["layers"]
    strip_height = 64

    seqs = [
        ["Attack", 64],
        ["Death", 64],
        ["Hurt", 64],
        ["Idle", 64],
        ["Run", 64],
        ["Walk", 64],
    ]

    lines = []
    dest_path_png = f"/home/cfrankb/toolkit/gcc/cs3-runtime-sdl/tools/sheet/data/{monster_name.lower()}/"
    # dest_path_config = f"/home/cfrankb/toolkit/gcc/cs3-runtime-sdl/tools/sheet/config/"
    dest_path_config = "data/"

    os.makedirs(dest_path_png, exist_ok=True)

    for seq_data in seqs:
        seq, frame_width = seq_data
        png_source_path = f"data/{monster_name}/{seq}/{monster_name}_{seq}_[LAYER].png"
        basename = f"{monster_name}_{seq}".lower()
        files, frames = process_strip(
            monster_name,
            png_source_path,
            layers,
            basename,
            dest_path_png,
            strip_height,
            frame_width,
        )
        files.sort()
        lines += [
            f"# {seq} = {frames} frames/seq ({frame_width}x{strip_height})",
            f"# total images: {frames*4}",
        ]
        lines += files + [""]
        print("")

        # shadow version
        layers1 = copy.deepcopy(layers)
        if "red" in layers:
            layers1.remove("red")
        basename = f"{monster_name}_{seq}s".lower()
        files, frames = process_strip(
            monster_name,
            png_source_path,
            ["shadow"] + layers1,
            basename,
            dest_path_png,
            strip_height,
            frame_width,
        )

    # print("\n".join(lines))
    out_list_file = f"{dest_path_config}{monster_name}.txt".lower()
    # print(out_list_file)

    with open(out_list_file, "w") as tfile:
        tfile.write("\n".join(lines))
