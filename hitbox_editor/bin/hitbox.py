import json
import glob
import os


class GridBox:
    GRID_SIZE = 8
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0
    type: int = 0

    def __init__(self, x: int, y: int, w: int, h: int, type: int):
        self.x = int(x / self.GRID_SIZE)
        self.y = int(y / self.GRID_SIZE)
        self.w = int(w / self.GRID_SIZE)
        self.h = int(h / self.GRID_SIZE)
        self.type = type

    def __str__(self) -> str:
        return f"[{self.x}, {self.y}, {self.w}, {self.h}, {self.type}]"

    def __repr__(self) -> str:
        return f"Box({self.x}, {self.y}, {self.w}, {self.h}, {self.type})"


def read_hitbox_file(input_file: str) -> dict[list[GridBox]]:

    sprites: dict[list[GridBox]] = dict()

    try:
        with open(input_file, "r") as file:
            data = json.load(file)
    except FileNotFoundError:
        print("Error: 'data.json' not found.")
        return {}
    except json.JSONDecodeError:
        print("Error: Invalid JSON format in 'data.json'.")
        return {}

    frame_width, frame_heigth = [data["frame"]["width"], data["frame"]["height"]]
    frames_per_row = data["frame"]["cols"]

    count = 0

    for hitbox in data["hitboxes"]:

        x = hitbox["x"]
        y = hitbox["y"]

        col = int(x / frame_width)
        row = int(y / frame_heigth)
        id = row * frames_per_row + col

        rel_x = x - col * frame_width
        rel_y = y - row * frame_heigth

        if hitbox["type"] == 0:
            #   print(id, hitbox, rel_x, rel_y)
            count += 1
            continue

        box: GridBox = GridBox(rel_x, rel_y, hitbox["w"], hitbox["h"], hitbox["type"])
        if id not in sprites:
            sprites[id] = []
        sprites[id].append(box)
        # print(box)

    print(sprites)
    print(os.path.basename(input_file), count)
    return sprites


path = "/home/cfrankb/toolkit/gcc/cs3-runtime-sdl/tools/sheet/data/beholder1/*.json"

for input_file in glob.glob(path):

    # input_file = "data/plant1_attack1.json"

    read_hitbox_file(input_file)
