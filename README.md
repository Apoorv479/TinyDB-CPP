# TinyDB: A Custom C++ Database Engine

A lightweight, fully functional Relational Database Engine built entirely from scratch in C++. This project demonstrates low-level system design, memory management, file I/O operations, and custom parsing logic without relying on any external database libraries like SQLite.

## Core Architecture & Features

- **File-per-Table Architecture:** Ensures robust data isolation and crash safety by dynamically generating and managing separate `.db` binary files for different tables (e.g., `.open users.db`).
- **SQL Projection (Column Filtering):** Dynamically filters output columns based on user queries to optimize memory access (e.g., `select id email`).
- **Abstract Expression Parsing:** Natively evaluates mathematical expressions directly via the `SELECT` command, acting as an internal calculator without hitting the disk (e.g., `select 200 * 5`).
- **SQL Aggregation Functions:** Scans table structures in memory to compute real-time data insights (`select count`, `select max`, `select min`, `select avg`).
- **Binary Disk Persistence:** Reads and writes C++ `structs` directly to disk in binary format for optimal I/O speed, ensuring data is saved across sessions.
- **Custom REPL (Read-Execute-Print-Loop):** An interactive command-line interface with built-in syntax validation and state security (prevents operations without an active database).

## Tech Stack

- **Language:** C++ (Standard Library)
- **Compiler:** GCC (MinGW / MSYS2)
- **Concepts Utilized:** Serialization, File Stream I/O (`<fstream>`), Tokenization (`<sstream>`), Enum-based State Machines, and Memory Management (`std::vector`).

## How to Run (Windows / Linux)

1. Clone the repository:
   ```bash
   git clone [https://github.com/your-username/TinyDB.git](https://github.com/your-username/TinyDB.git)
   cd TinyDB/src
   ```
   Compile the engine:Bashg++ main.cpp -o tinydb
   Run the executable:Bash./tinydb
   Usage & DemoStart the database engine and interact using the custom REPL:SQLdb > .open users.db -- Creates or Opens the 'users' table
   db > insert 1 apoorv a@a.com -- Inserts a record (Format: id username email)
   db > insert 2 rohit r@r.com -- Inserts a second record

db > select -- Views all records: (1, apoorv, a@a.com)
db > select email username -- Tests Projection: (apoorv, a@a.com)
db > select max -- Tests Aggregation: Max ID: 2
db > select 1500 / 30 -- Tests Math Parser: Result: 50

db > .open products.db -- Safely saves 'users' and switches context
db > .exit -- Saves current data and gracefully shuts down

## Repository StructurePlaintextTinyDB/

│
├── src/
│ └── main.cpp # The final, feature-complete database engine
│
├── previous-phases/ # Evolution of the architecture for reference
│ ├── phase1.cpp # Basic command loop
│ ├── phase2.cpp # Tokenizer and AST basics
│ ├── phase3.cpp # In-memory storage using Vectors
│ └── phase4.1.cpp # Hardcoded single-file persistence
│
└── README.md

## Future EnhancementsImplementing a dynamic schema parser (CREATE TABLE) with a System Catalog header.Adding an In-Memory Hash Index (B-Tree alternative) for $O(1)$ search lookups.
