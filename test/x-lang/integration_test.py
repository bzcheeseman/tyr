from path import *


def check_serialized(serialized: List[int], serialized_size: int) -> bool:
    deserialized = deserialize_path(serialized, serialized_size)

    idx = get_path_idx(deserialized)
    assert idx[0] and idx[1] == 5

    x_count = get_path_x_count(deserialized)
    assert x_count[0] and x_count[1] == 125

    y_count = get_path_y_count(deserialized)
    assert y_count[0] and y_count[1] == 125

    destroy_path(deserialized)

    return True


if __name__ == "__main__":
    serialized = None
    with open("serialized_path.tsf", "rb") as f:
        serialized = f.read()

    serialized_size = len(serialized)
    # just make sure you can deserialize and the counts are correct
    assert check_serialized(serialized, serialized_size)
    print("Test succeeded")
