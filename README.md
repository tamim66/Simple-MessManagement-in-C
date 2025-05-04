Simple C project for (Bangladesh University of Business and technology-BUBT) SDP-1 course 
^_____^
![](https://github.com/tamim66/Simple-MessManagement-in-C/blob/master/%7B74E578AF-F274-4BCA-B60B-44FBF118C8A9%7D.png?raw=true)

A simple terminal-based Mess Management System built in C using MySQL for backend storage. Designed for bachelor messes in Bangladesh, this system allows you to manage members, log meals, record payments, and view summaries with automated expense tracking.
### Features
- Add/Delete Users
- Log Daily Meals (with adjustable rate)
- Record Payments
- View Total Expenses
- View User-wise Summary (Meals, Payments, Dues)

### Requirements
- GCC (C Compiler)
- MySQL Server(XAMPP)
- MySQL C Connector (libmysqlclient)
- Visual Studio C/C++ IDE and Compiler for Windows
  
### Download the MySQL Connector/C
- Go to: [https://dev.mysql.com/downloads/connector/c/](https://dev.mysql.com/downloads/connector/c/)
- Download the **ZIP Archive (Windows, x64)**.
- Extract it to:  
  `C:\mysql-connector-c-6.1.11-winx64`

### Configure Visual Studio Project
- Create a new **Console App** (C language).
- Open **Project → Properties** and configure:

#### ➕ C/C++ → General → Additional Include Directories: 
`C:\mysql-connector-c-6.1.11-winx64\include`
#### ➕ Linker → General → Additional Library Directories: 
`C:\mysql-connector-c-6.1.11-winx64\lib`
#### ➕ Under Linker → Input → Additional Dependencies: 
`libmysql.lib`

### Copy `libmysql.dll` to your project’s **output directory** (e.g., `Debug/` or `Release/`).




