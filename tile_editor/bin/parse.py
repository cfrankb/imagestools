import json

with open("data/777.json") as sfile:
    data = json.load(sfile)


rows = []

for tile in data["tiles"]:

    if tile["tag"]:
        print(tile)

    row = [
        str(tile["index"]),
        "0" if tile["next"] < 0 else str(tile["next"]),
        str(tile["speed"]),
        str(tile["type"]),
        tile["tag"],
    ]
    rows.append("\t".join(row))

with open("data/layerdata.tsv", "w") as t:
    t.write("\n".join(rows))
