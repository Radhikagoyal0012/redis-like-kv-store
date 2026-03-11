# redis-like-kv-store
# Redis-like In-Memory Key-Value Store

## Overview

This project implements a simplified **Redis-like in-memory key-value store** written in **C++**.
It supports basic data operations, TTL-based key expiration, pattern-based key lookup, persistence through snapshots, and thread-safe concurrent access.

The goal of the project is to simulate the core functionality of a lightweight in-memory database similar to Redis.

---

# Features

Supported commands:

```
SET key value [EX seconds]
GET key
DEL key
KEYS pattern
TTL key
SAVE
LOAD
STATS
```

### Command Descriptions

**SET key value [EX seconds]**

Stores a key-value pair in memory.

Example:

```
SET user:1 Alice EX 300
```

Returns:

```
OK
```

---

**GET key**

Retrieves the value associated with a key.

Example:

```
GET user:1
```

Output:

```
Alice
```

If the key does not exist:

```
(nil)
```

---

**DEL key**

Deletes a key from the store.

Example:

```
DEL user:1
```

Output:

```
OK
```

---

**KEYS pattern**

Returns all keys matching a glob-style pattern.

Example:

```
KEYS user:*
```

Output:

```
user:1
user:2
```

---

**TTL key**

Returns remaining time-to-live for a key.

Possible responses:

| Output | Meaning                      |
| ------ | ---------------------------- |
| >0     | seconds remaining            |
| -1     | key exists but has no expiry |
| -2     | key does not exist           |

---

**SAVE**

Writes all non-expired keys to a snapshot file (`snapshot.json`).

---

**LOAD**

Restores keys from the snapshot file.

---

**STATS**

Displays system statistics:

* Total keys
* Expired keys cleaned
* Estimated memory usage

---

# Design Decisions

## Data Structures

The following data structures are used:

```cpp
unordered_map<string, string> store
unordered_map<string, time_point> expiry
```

### Reasoning

* `unordered_map` provides **O(1) average time complexity** for lookups and insertions.
* A separate expiry map keeps track of expiration times without affecting normal key operations.

---

## Expiration Strategy

The store uses a **two-layer expiration mechanism**.

### Lazy Expiration

Whenever a key is accessed (GET, TTL, KEYS), the program checks whether the key has expired.
If it has expired, the key is removed immediately.

### Periodic Cleanup

A background thread runs every **1 second** and removes expired keys from the store.

This ensures memory does not grow unnecessarily with expired keys.

---

## Thread Safety

To support concurrent access safely, all key-value operations are protected using:

```
std::mutex
```

Each operation acquires a lock using `std::lock_guard`, preventing race conditions.

---

## Persistence

The `SAVE` command writes the current key-value data into a **JSON snapshot file**.

Example snapshot:

```json
{
  "user:1": "Alice",
  "counter": "10"
}
```

The `LOAD` command reads the snapshot and restores the data into memory.

---

# Build Instructions

Compile the program using a C++17 compatible compiler.

Example:

```
g++ -std=c++17 src/main.cpp -pthread -o kvstore
```

---

# Running the Program

Run the executable:

```
./kvstore
```

(Windows PowerShell)

```
.\kvstore.exe
```

The program will then accept commands through standard input.

---

# Example Usage

Input:

```
SET user:1 Alice EX 300
GET user:1
TTL user:1
SET counter 0
KEYS user:*
DEL user:1
GET user:1
```

Output:

```
OK
Alice
299
OK
user:1
OK
(nil)
```

---

# Memory Usage Estimate

The `STATS` command provides a rough memory estimate based on:

```
size of keys + size of values
```

This does not include container overhead but provides a simple approximation.

---

# Known Limitations / Trade-offs

1. The snapshot persistence is simplistic and does not store TTL information.
2. Pattern matching for KEYS uses regex conversion from glob patterns, which may not fully match Redis behavior.
3. The memory usage reported is only an estimate.
4. The server currently accepts commands through **stdin** rather than a TCP socket interface.

---

# Possible Improvements

Future enhancements could include:

* TCP socket server for multiple client connections
* LRU eviction policy when memory limit is reached
* List operations (LPUSH, RPOP)
* Pub/Sub messaging system
* More efficient expiration using a priority queue (min-heap)

---

# Author

Radhika Goyal

---

# License

This project is for assessment and educational purposes.
