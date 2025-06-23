# HydroSQL

HydroSQL is a lightweight relational database system that supports basic SQL (Structured Query Language) operations. It features a client-server architecture where the server can connected with multiple client at the same time.

## Getting Started
### Connect the Server

To begin, connect to the server using its IP address and port:

```
Connect server:
Server IP: 127.0.0.1
Server Port: 8080
Connecting server 127.0.0.1:8080.
```

### User Authentication

After successfully connected, the server will ask you to log in or sign up：

Sign up:
```
Connected to server 127.0.0.1:8080.
(L)Log In
(S)Sign Up
> s
Username:
> username
Password:
> *********
Please enter again:
> *********
Sign up success.
```
Your password will be hashed using Argon2id and the hashed password will be stored in the server.

Log in:
```
Connected to server 127.0.0.1:8080.
(L)Log In
(S)Sign Up
> l
Username:
> username
Password:
> *********
Log in success.
```
The server will search and send the stored password hash. Then the client can verify the password entered.

### Database Operations

#### Supported Data Types

| Type | Description |
|---|---|
| INT | 4-byte integer |
| SMALLINT | 2-byte integer |
| BIGINT | 8-byte integer |
| FLOAT | Single-precision floating-point |
| DECIMAL | Double-precision floating-point |
| CHAR | Single-byte character |
| VARCHAR | UTF-8 string (with length specifier) |
| BOOLEAN | 1-byte boolean |
| DATE | Date format (YYYY-MM-DD) |
| TIME |Time format (HH:MM:SS) |
| DATETIME | Combined date and time (YYYY-MM-DD-HH:MM:SS) |

#### Table Management

The new-sign-up user can not access all of the existing database. You can whether ask other user who has administrator permissions to grant you an permission, or create a table youself.

#### Create Table

```SQL
> CREATE TABLE employees
> index BIGINT
> name VARCHAR(10)
> birthday DATE
> salary FLOAT;
[SUCCESS] Create table employees.
```
The one who create table will become the administrator of that table automatically, which means you can read, modify or delete the table you create.

Note that each SQL command should end with a semicolon.

#### Insert Data

Single row:
```SQL
> INSERT INTO employees
> (index, name, birthday, salary)
> VALUES
> (1, "张三", 2001-05-14, 10000.5);
[SUCCESS] 1 row(s) inserted.
```

Multiple rows:
```SQL
> INSERT INTO employees
> (index, name, salary)
> VALUES
> (2, "李四", 15000),
> (3, "王五", 17777),
> (4, "Sam Smith", 13000);
[SUCCESS] 3 row(s) inserted.
```
The column birthday will be fill with default value.

Insert using expression:
```SQL
> INSERT INTO employees
> (index, name, salary)
> VALUES
> (5, "Tom", index * 2000 + 5000.5);
[SUCCESS] 1 row(s) inserted.
```

#### Query Data

Basic select:
```SQL
> SELECT * FROM employees;
[SUCCESS] 5 row(s) selected.
index   name        birthday        salary
1       张三        2001-05-14      10000.500000
2       李四        0000-00-00      15000.000000
3       王五        0000-00-00      17777.000000
4       Sam Smith  0000-00-00      13000.000000
5       Tom        0000-00-00      15000.500000
```

Conditional select:
```SQL
> SELECT (index, name, salary) FROM employees
> WHERE index > 4 OR name = "张三";
[SUCCESS] 2 row(s) selected.
index   name    salary
1       张三    10000.500000
5       Tom     15000.500000
```

#### Update Data

```SQL
> UPDATE employees
> SET name = "James", salary = salary * 2 + index
> WHERE index = 5;
[SUCCESS] 1 row(s) updated.
> SELECT (index, name, salary) FROM employees
> WHERE index = 5;
[SUCCESS] 1 row(s) selected.
index   name    salary
5       James   30006.000000
```

#### Delete Data

```SQL
> DELETE FROM employees
> WHERE salary < 12000 OR name = "Sam Smith";
[SUCCESS] 2 row(s) deleted.
> SELECT * FROM employees;
[SUCCESS] 3 row(s) selected.
index   name    birthday        salary
2       李四    0000-00-00      15000.000000
3       王五    0000-00-00      17777.000000
5       James   0000-00-00      30006.000000
```

#### Access Control

```SQL
> GRANT READONLY ON employees TO user2, user3;
[SUCCESS] 2 users' permission levels updated.
```

Four permission levels:

1. NULL: No access

2. READONLY: SELECT only

3. MODIFY: SELECT, INSERT, UPDATE, DELETE

4. ADMIN: Full access including GRANT

#### Drop
```SQL
> DROP TABLE employees;
[SUCCESS] 1 table dropped.
```

#### Quit

```
> QUIT;
Disconnected from server.
Press any key to exit.
```

## Example: Comprehensive Vehicle Manage System

Administrator:
```SQL
Connect server:
Server IP: 127.0.0.1
Server Port: 8080
Connecting server 127.0.0.1:8080.
Connected to server 127.0.0.1:8080.
(L)Log In
(S)Sign Up
> l
Username:
> admin
Password:
> *********
Log in success.
> CREATE TABLE vehicles
> registration_number VARCHAR(12)
> vehicle_type VARCHAR(20)
> color VARCHAR(10)
> date_of_manufacture DATE;
[SUCCESS] Create table vehicles.
> INSERT INTO vehicles
> (registration_number, vehicle_type, color, date_of_manufacture)
> VALUES
> ("青F3B0B0", "SUV", "white", 2015-11-24),
> ("桂LCQ545", "Sports", "red", 2023-02-04),
> ("桂KM6434", "Sedan", "black", 2020-05-13),
> ("津GFX5009", "Truck", "blue", 2012-07-23),
> ("豫U2YM69", "Trailer", "green", 2017-04-30);
[SUCCESS] 5 row(s) inserted.
> DELETE FROM vehicles
> WHERE registration_number = "桂KM6434";
[SUCCESS] 1 row(s) deleted.
> UPDATE vehicles
> SET date_of_manufacture = 2013-11-20
> WHERE registration_number = "桂LCQ545";
[SUCCESS] 1 row(s) updated.
> SELECT * FROM vehicles
> WHERE registration_number = "桂LCQ545" OR vehicle_type = "SUV";
[SUCCESS] 2 row(s) selected.
registration_number     vehicle_type    color   date_of_manufacture
青F3B0B0        SUV     white   2015-11-24
桂LCQ545        Sports  red     2013-11-20
> GRANT READONLY ON vehicles TO normal;
[SUCCESS] 1 users' premission level updated.
> QUIT;
Disconnected from server.
Press any key to exit.
```

Normal user:
```SQL
Connect server:
Server IP: 127.0.0.1
Server Port: 8080
Connecting server 127.0.0.1:8080.
Connected to server 127.0.0.1:8080.
(L)Log In
(S)Sign Up
> l
Username:
> normal
Password:
> *********
Log in success.
> SELECT * FROM vehicles
> WHERE registration_number = "豫U2YM69";
[SUCCESS] 1 row(s) selected.
registration_number     vehicle_type    color   date_of_manufacture
豫U2YM69        Trailer green   2017-04-30
> INSERT INTO vehicles
> (registraion_number, color)
> VALUES
> ("粤TKB7362", "white");
[FAILED] Premission Insufficient.
> UPDATE vehicles
> SET color = "black";
[FAILED] Premission Insufficient.
> DELETE FROM vehicles
> WHERE vehicle_type = "Truck";
[FAILED] Premission Insufficient.
> QUIT;
Disconnected from server.
Press any key to exit.
```

## Further Plan... if there is.

- OPTIMIZE TABLE statement: Disk defragmentation.
- Column constraints: PRIMERY KEY, UNIQUE, DEFAULT...etc.
- Better TCP connection: Add timeover detecion and heart beat detection.
- Table Index: Utilizing B-trees.

## Dependencies

[libsodium](https://github.com/jedisct1/libsodium) for Argon2id hashing.
