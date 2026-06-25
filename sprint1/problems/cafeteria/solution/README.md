## The company's unique recipe:

- Bake the bun for 1 second and the sausage for 1.5 seconds.
- Assemble a hot dog from them. The acceptable cooking time for a bun is 1-1.5 seconds, and for a sausage, it's 1.5-2 seconds. If you go outside these limits in either direction, the hot dog will be rejected.

Such fast hot dog cooking times are provided by the unique “Die Ablo” gas stove, whose flame temperature is 3000 degrees Celsius.

These stoves are a custom item, so the cafeteria has only one gas stove with 8 burners. Each burner can heat one bun or fry one sausage at a time. In real-world production tasks, such limitations are also possible. For example, a web application might have a limit on the maximum number of simultaneous database connections.

- `GasCooker` (file `gascooker.h`) — ready-made class for controlling the gas stove. Methods of this class can be called from multiple threads simultaneously.
- `GasCookerLock` (file `gascooker.h`) — ready-made RAII wrapper for using `GasCooker` resources.
- Ready-made `Sausage` class and partially implemented `Bread` class, managing the preparation of sausages and buns. They are located in the `ingredients.h` file.
- `Store` (file `ingredients.h`) — ready-made class for issuing sausages and buns from the warehouse.
- `HotDog` (file `hotdog.h`) — ready-made class that assembles a hot dog from available ingredients and performs quality control.
- Auxiliary template class `Result` (file `result.h`) for storing a value or an error that occurred.
- Skeleton of the `Cafeteria` class (file `cafeteria.h`).

Leave the code of the classes listed above, except for `Cafeteria` and `Bread`, unchanged. You can create your own classes and functions in addition to the existing ones.

Implement the `OrderHotDog` method in the `Cafeteria` class, which prepares a hot dog according to the described recipe. Complete the missing functionality in the `Bread` class by analogy with the `Sausage` class.
Mandatory requirement: it must be possible to call the `Cafeteria::OrderHotDog` method from several worker threads simultaneously. To do this, use the approaches described in the lesson to eliminate race conditions and techniques from the provided classes.
The `main.cpp` file contains acceptance tests that check the operation of the `Cafeteria` class. The code you develop must pass all checks, including those that measure the execution time of `N` orders on a given number of worker threads.
