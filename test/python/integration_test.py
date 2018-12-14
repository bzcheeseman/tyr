import numpy as np
from path import *


def get_serialized_storage() -> (List[int], int, np.ndarray):
    storage = create_path(5)

    x = np.random.rand(152)
    y = np.random.rand(152)

    assert set_path_x(storage, list(x.tolist()), len(x))
    assert set_path_y(storage, list(y.tolist()), len(y))

    serialized = serialize_path(storage)

    destroy_path(storage)

    return serialized, x, y


def check_serialized(serialized: List[int], array_x: np.ndarray, array_y: np.ndarray) -> bool:
    deserialized = deserialize_path(serialized)

    idx = get_path_idx(deserialized)
    assert idx[0] and idx[1] == 5

    x_count = get_path_x_count(deserialized)
    assert x_count[0] and x_count[1] == 152

    y_count = get_path_y_count(deserialized)
    assert y_count[0] and y_count[1] == 152

    x = get_path_x(deserialized)
    x = ptr_to_array(x[1], x_count[1])
    y = get_path_y(deserialized)
    y = ptr_to_array(y[1], y_count[1])
    assert x[0] and y[0] and np.allclose(x, array_x) and np.allclose(y, array_y)

    destroy_path(deserialized)

    return True


if __name__ == "__main__":
    serialized, x, y = get_serialized_storage()
    assert check_serialized(serialized, x, y)
    print("Test succeeded")
