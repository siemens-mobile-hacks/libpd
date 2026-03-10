# libpd – Parser for .pd configuration files

libpd is a lightweight C library for parsing and generating `.pd` configuration files. The format is inspired by configuration files used in **Siemens** mobile phones. It provides a simple API to read, manipulate, and write key‑value pairs with optional grouping.

## File Format

```
000024:T:SHOW_FUNCTION=1
000037:T:FAK_KEY_STAR.FLAG_READONLY=1
```

Each line in a `.pd` file has the following structure:
- **First 6 characters** – the total length of the line excluding the newline characters (including the 6 digits, the `":T:"` marker, the parameter name, the `'='` sign, and the value).
- **`:T:`** – fixed marker indicating the start of the actual data.
- **Parameter name** – can be either `key` alone (e.g., `SHOW_FUNCTION`) or `group.key` (e.g., `FAK_KEY_STAR.FLAG_READONLY`).
- **`=`** – separates name and value.
- **Value** – can be an integer or a string.
- Lines end with `\r\n`, but the length prefix does **not** include these characters.

Whitespace lines are ignored.

## API Reference

```C
int pd_read_file(const char *file_name, pd_node_t ***nodes);
```
Reads a `.pd` file and creates an array of nodes.

- Parameters
  - `file_name` – path to the input file.
  - `nodes` – output parameter. On success, `*nodes` points to a newly allocated NULL‑terminated array of `pd_node_t*` pointers.
- Returns
  - `0` on success.
  - `-1` if the file cannot be opened.
  - A positive line number if a parsing error occurs on that line.
  - Negative values for other I/O or memory errors.

```C
int pd_write_file(const char *file_name, const pd_node_t **nodes);
```
Writes an array of nodes to a file in the `.pd` format.

- Parameters
  - `file_name` – path to the output file.
  - `nodes` – NULL‑terminated array of node pointers.
- Returns
  - `0` on success.
  - `-1` if the file cannot be created.
  - `-2` if a write error occurs.

```C
void pd_free_nodes(pd_node_t **nodes);
```
Frees all memory allocated for the node array, including the nodes themselves and the array.
- Parameters
  - `nodes` – pointer to a NULL‑terminated array previously obtained from `pd_read_file` (or created manually). After the call `*nodes` is set to NULL.
