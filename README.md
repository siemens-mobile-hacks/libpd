# libpd – Parser for .pd configuration files

libpd is a lightweight C library for parsing and generating `.pd` configuration files. The format is inspired by configuration files used in **Siemens** mobile phones. It provides a simple API to read, manipulate, and write key‑value pairs with optional grouping.

## File Format

```
000024:T:SHOW_FUNCTION=1
000037:T:FAK_KEY_STAR.FLAG_READONLY=1
000020:T:EmptyValue=
```

Each line in a `.pd` file has the following structure:
- **First 6 characters** – the total length of the line excluding the newline characters (including the 6 digits, the `":T:"` marker, the parameter name, the `'='` sign, and the value).
- **`:T:`** – fixed marker indicating the start of the actual data.
- **Parameter name** – can be either `key` alone (e.g., `SHOW_FUNCTION`) or `group.key` (e.g., `FAK_KEY_STAR.FLAG_READONLY`).
- **`=`** – separates name and value.
- **Value** – can be an integer, a string, or **empty** (e.g., EmptyValue=).
- Lines end with `\r\n`, but the length prefix does **not** include these characters.

Whitespace lines are ignored.

## Building
The SDK must be located at `../../sdk` relative to the project root.
```bash
mkdir build && cd build
cmake ..
make
```

## API Reference

```C
pd_node_t *pd_alloc_integer_node(const char *group, const char *key, long value);
```
Creates a new node holding an integer value.

- Parameters
  - `group` - group name (may be `NULL` or an empty string if no group is needed). The length must not exceed `PD_MAX_GROUP_SIZE`.
  - `key` - key name (must not be `NULL` or empty, otherwise the function fails). The length must not exceed `PD_MAX_KEY_SIZE`.
  - `value` - integer value to be stored in the node.

Returns a pointer to the newly allocated `pd_node_t` structure or `NULL`.

```C
pd_node_t *pd_alloc_string_node(const char *group, const char *key, const char *value);
```
Creates a new node holding a string value.

- Parameters
  - `group` - group name (may be `NULL` or an empty string if no group is needed). The length must not exceed `PD_MAX_GROUP_SIZE`.
  - `key` - key name (must not be `NULL` or empty, otherwise the function fails). The length must not exceed `PD_MAX_KEY_SIZE`.
  - `value` - string to store. If `NULL` or an empty string, an empty string is stored in the node. The length (if non‑empty) must not exceed `PD_MAX_VALUE_SIZE`.

Returns a pointer to the newly allocated `pd_node_t` structure or `NULL`.

```C
int pd_read_file(const char *file_name, pd_node_t ***nodes);
```
Reads a `.pd` file and creates an array of nodes.

- Parameters
  - `file_name` - path to the input file.
  - `nodes` - output parameter. On success, `*nodes` points to a newly allocated NULL‑terminated array of `pd_node_t*` pointers.
- Returns
  - `0` on success (including the case of an empty file, in which case a minimal array containing only a `NULL` pointer is allocated).
  - `-1` if the file cannot be opened.
  - A positive line number if a parsing error occurs on that line.
  - Negative values for other I/O or memory errors.

```C
int pd_write_file(const char *file_name, const pd_node_t **nodes);
```
Writes an array of nodes to a file in the `.pd` format.

- Parameters
  - `file_name` - path to the output file.
  - `nodes` - NULL‑terminated array of node pointers.
- Returns
  - `0` on success.
  - `-1` if the file cannot be created.
  - `-2` if a write error occurs.

```C
size_t pd_get_max_group_size(void);
```
Returns the maximum allowed length for a group name (excluding the null terminator).

```C
size_t pd_get_max_key_size(void);
```
Returns the maximum allowed length for a key name (excluding the null terminator).

```C
size_t pd_get_max_value_size(void);
```
Returns the maximum allowed length for a string value (excluding the null terminator).

```C
size_t pd_get_size(pd_node_t **nodes);
```
Counts the number of nodes in a NULL‑terminated array.

- Parameters
  - `nodes` - pointer to a NULL‑terminated array of `pd_node_t*` pointers.

Returns the number of nodes in the array (excluding the terminating `NULL`).

```C
void pd_add_node(pd_node_t ***nodes, pd_node_t *node);
```
Appends a node to the end of a dynamically allocated NULL‑terminated array.
- Parameters
  - `nodes` - pointer to the array pointer.
  - `node` - pointer to the node to add (must not be `NULL`).

```C
void pd_delete_node(pd_node_t ***nodes, size_t id);
```
Removes a node at the specified index from a NULL‑terminated array.
- Parameters
  - `nodes` - pointer to the array pointer.
  - `id` - zero‑based index of the node to delete.

```C
void pd_free_node(pd_node_t *node);
```
Frees memory allocated for a single node.
- Parameters
  - `node` - pointer to the node to free.

```C
void pd_free_nodes(pd_node_t **nodes);
```
Frees all memory allocated for the node array, including the nodes themselves and the array.
- Parameters
  - `nodes` - pointer to a NULL‑terminated array previously obtained from `pd_read_file` (or created manually). After the call, the array and all nodes are freed.
