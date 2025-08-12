# SQLMINT : SQL Database Engine

A complete SQL database engine built from scratch in C++17. This database can create tables, store data, and run queries just like a real database, but it's much simpler and easier to understand.

## What This Project Does

This is a working database that you can use to:
- Create tables with different types of data
- Insert information into those tables
- Search and filter your data
- All data is saved to files so it persists between sessions

## How to Build and Run

### Requirements
- A C++ compiler that supports C++17 (like g++ version 7 or newer)
- Make build system

### Building the Database
```bash
# Build the application
make all

# Or build and run directly
make run

# Clean up build files if needed
make clean
```

### Starting the Database
```bash
./sqldb
```

You'll see a prompt that looks like this:
```
SQL Database Engine v1.0
========================
Type 'help' or '\h' for help, '\q' to quit.

sqldb> 
```

## How to Use the Database

### Creating Tables

To create a new table, use the CREATE TABLE command:

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name VARCHAR(50),
    age INTEGER,
    active BOOLEAN
);
```

This creates a table called "users" with four columns:
- `id`: A number that uniquely identifies each user
- `name`: Text up to 50 characters long
- `age`: A number for the person's age
- `active`: True or false value

### Adding Data

To add information to your table:

```sql
INSERT INTO users VALUES (1, 'Alice Johnson', 28, true);
INSERT INTO users VALUES (2, 'Bob Smith', 35, false);
INSERT INTO users VALUES (3, 'Carol Brown', 22, true);
```

### Getting Data Back

To see all the data in a table:

```sql
SELECT * FROM users;
```

This will show you a nice table with all your data:
```
| id         | name         | age        | active     |
+------------+--------------+------------+------------+
| 1          | Alice Johnson| 28         | true       |
| 2          | Bob Smith    | 35         | false      |
| 3          | Carol Brown  | 22         | true       |
3 rows returned.
```

### Filtering Data

You can search for specific information using WHERE:

```sql
-- Find users older than 25
SELECT * FROM users WHERE age > 25;

-- Find active users
SELECT * FROM users WHERE active = true;

-- Find a specific user by ID
SELECT * FROM users WHERE id = 2;
```

### DROP table

You can delete the table using DROP.

```sql
DROP TABLE name_of_table;
```

## Data Types You Can Use

### INTEGER
Regular numbers like 1, 42, -10, 1000

Example:
```sql
CREATE TABLE products (price INTEGER);
INSERT INTO products VALUES (999);
```

### VARCHAR(n)
Text with a maximum length. Replace `n` with the maximum number of characters.

Example:
```sql
CREATE TABLE messages (content VARCHAR(200));
INSERT INTO messages VALUES ('Hello World');
```

### BOOLEAN
True or false values

Example:
```sql
CREATE TABLE settings (enabled BOOLEAN);
INSERT INTO settings VALUES (true);
INSERT INTO settings VALUES (false);
```

## Special Rules (Constraints)

### PRIMARY KEY
Marks a column as the main identifier for each row. You can only have one primary key per table.

```sql
CREATE TABLE employees (
    emp_id INTEGER PRIMARY KEY,
    name VARCHAR(100)
);
```

### NOT NULL
Means the column must always have a value.

```sql
CREATE TABLE customers (
    id INTEGER PRIMARY KEY,
    email VARCHAR(100) NOT NULL
);
```

## Comparison Operators

When filtering data with WHERE, you can use these operators:
- `=` : equals
- `!=` or `<>` : not equals
- `<` : less than
- `>` : greater than
- `<=` : less than or equal
- `>=` : greater than or equal

## Helpful Commands

### See All Your Tables
```
\l
```
or
```
\list
```

This shows all tables and their structure.

### Get Help
```
\h
```
or
```
help
```

### Exit the Database
```
\q
```
or
```
exit
```
or
```
quit
```

## Complete Example Session

Here's a full example of using the database:

```sql
sqldb> CREATE TABLE employees (id INTEGER PRIMARY KEY, name VARCHAR(100), salary INTEGER);
Table 'employees' created successfully.

sqldb> INSERT INTO employees VALUES (1, 'John Doe', 50000);
1 row inserted into 'employees'.

sqldb> INSERT INTO employees VALUES (2, 'Jane Smith', 60000);
1 row inserted into 'employees'.

sqldb> SELECT * FROM employees;
| id         | name       | salary     |
+------------+------------+------------+
| 1          | John Doe   | 50000      |
| 2          | Jane Smith | 60000      |
2 rows returned.

sqldb> SELECT * FROM employees WHERE salary > 55000;
| id         | name       | salary     |
+------------+------------+------------+
| 2          | Jane Smith | 60000      |
1 rows returned.

sqldb> \l
Tables:
=======
  employees
    Columns:
      id INTEGER PRIMARY KEY
      name VARCHAR(100)
      salary INTEGER

sqldb> exit
Goodbye!
```

## Real-World Examples

### Online Store Inventory
```sql
-- Create a products table
CREATE TABLE products (
    product_id INTEGER PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    price INTEGER,
    in_stock BOOLEAN
);

-- Add some products
INSERT INTO products VALUES (1, 'Gaming Laptop', 1299, true);
INSERT INTO products VALUES (2, 'Wireless Mouse', 25, true);
INSERT INTO products VALUES (3, 'Mechanical Keyboard', 89, false);

-- Find available products under $100
SELECT * FROM products WHERE price < 100 AND in_stock = true;
```

### Customer Database
```sql
-- Create a customers table
CREATE TABLE customers (
    customer_id INTEGER PRIMARY KEY,
    email VARCHAR(100) NOT NULL,
    age INTEGER,
    premium_member BOOLEAN
);

-- Add customers
INSERT INTO customers VALUES (1, 'alice@email.com', 28, true);
INSERT INTO customers VALUES (2, 'bob@email.com', 35, false);

-- Find premium members
SELECT * FROM customers WHERE premium_member = true;
```

## Where Your Data is Stored

All your data is automatically saved in a `data/` folder:
- `data/metadata.db` - Information about your tables
- `data/tablename.tbl` - The actual data for each table

Your data will still be there when you restart the database.

## Project Structure

```
sql-database-engine/
├── src/
│   ├── main.cpp           # Main program
│   ├── common/
│   │   └── types.h        # Data types and structures
│   ├── parser/
│   │   ├── tokenizer.h    # Breaks SQL into pieces
│   │   ├── tokenizer.cpp
│   │   ├── parser.h       # Understands SQL commands
│   │   └── parser.cpp
│   ├── storage/
│   │   ├── metadata.h     # Manages table information
│   │   ├── metadata.cpp
│   │   ├── table.h        # Handles data storage
│   │   └── table.cpp
│   └── executor/
│       ├── query_executor.h  # Runs SQL commands
│       └── query_executor.cpp
├── obj/                   # Build files (created automatically)
├── data/                  # Your database files (created automatically)
├── Makefile              # Build instructions
└── README.md            # This file
```

## What This Database Can Do

**Supported Features:**
- CREATE TABLE with columns and constraints
- INSERT data into tables
- SELECT data with WHERE filtering
- Data types: INTEGER, VARCHAR, BOOLEAN
- Constraints: PRIMARY KEY, NOT NULL
- Comparison operators: =, !=, <, >, <=, >=
- Data persistence (saves to files)
- Interactive shell with help commands

## What This Database Cannot Do (Yet)

**Not Supported:**
- JOIN operations between tables
- Multiple table operations
- Complex WHERE clauses (only one condition at a time)
- UPDATE or DELETE statements
- Transactions
- Indexes for faster searches
- Multiple users at the same time

## Error Messages

If something goes wrong, the database will tell you what happened:

```sql
sqldb> CREATE TABLE users (id INTEGER);
sqldb> CREATE TABLE users (name VARCHAR(50));
Table 'users' already exists

sqldb> INSERT INTO nonexistent VALUES (1);
Table 'nonexistent' does not exist

sqldb> INSERT INTO users VALUES (1, 2);  -- if table has only 1 column
INSERT has 2 values, expected 1
```

## Some Rules

1. **Always end SQL commands with a semicolon (;)**
2. **Use single quotes for text:** `'Hello World'` not `"Hello World"`
3. **Table and column names are case-sensitive**

