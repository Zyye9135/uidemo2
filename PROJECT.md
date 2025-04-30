# GNCDB UI 项目文档

## 项目概述
GNCDB UI 是一个基于 Qt 的数据库管理工具，提供了图形化界面来管理和操作 GNCDB 数据库。该项目支持数据库的基本操作，包括表的创建、修改、删除，以及数据的增删改查等功能。

## 项目结构

### 主要类

#### MainWindow 类
主窗口类，负责整个应用程序的界面和功能实现。

##### 主要功能
1. 数据库连接管理
   - `onConnectDB()`: 连接数据库
   - `onDisconnectDB()`: 断开数据库连接
   - `onNewDatabase()`: 创建新数据库

2. 表管理
   - `onShowTables()`: 显示表列表
   - `onRefreshTables()`: 刷新表列表
   - `onCreateTable()`: 创建新表
   - `onDropTable()`: 删除表
   - `updateTableList()`: 更新表列表显示

3. 数据操作
   - `onAddRow()`: 添加新行
   - `onEditRow()`: 编辑行
   - `onDeleteRow()`: 删除行
   - `executeInsertSQL()`: 执行插入操作
   - `executeUpdateSQL()`: 执行更新操作
   - `executeDeleteSQL()`: 执行删除操作

4. SQL 操作
   - `onExecuteSQL()`: 执行 SQL 语句
   - `onExecuteCurrentLine()`: 执行当前行 SQL
   - `onFormatSQL()`: 格式化 SQL
   - `executeSQLStatement()`: 执行单个 SQL 语句

5. 界面管理
   - `initUI()`: 初始化用户界面
   - `initConnections()`: 初始化信号槽连接
   - `onShowTreeView()`: 显示树形视图
   - `onShowDatabaseTab()`: 显示数据库标签页
   - `onShowDataTab()`: 显示数据标签页
   - `onShowDDLTab()`: 显示 DDL 标签页
   - `onShowSQLTab()`: 显示 SQL 标签页

#### DBManager 类
数据库管理类，负责数据库连接和基本操作。

##### 主要功能
- `open()`: 打开数据库连接
- `close()`: 关闭数据库连接
- `isOpen()`: 检查数据库是否打开
- `executeQuery()`: 执行查询
- `getDB()`: 获取数据库连接

#### TreeItemDelegate 类
树形视图项代理类，负责自定义树形视图项的显示。

##### 主要功能
- `paint()`: 绘制树形视图项
- `sizeHint()`: 计算项的大小

#### SqlHighlighter 类
SQL 语法高亮类，负责 SQL 语句的语法高亮显示。

##### 主要功能
- `highlightBlock()`: 高亮显示 SQL 语句块
- `setTheme()`: 设置高亮主题
- `addCustomKeywords()`: 添加自定义关键字

### 数据结构

#### TableInfo 结构体
存储表的基本信息。

#### SQLResult 结构体
存储 SQL 查询结果。

### 回调函数

1. `selectCallback()`
   - 用于表选择操作的回调函数
   - 处理表列表的查询结果

2. `sqlResultCallback()`
   - 用于 SQL 查询结果处理的回调函数
   - 处理查询结果的列名和数据

3. `cppCallback()`
   - C++ 回调函数
   - 处理特定的数据库操作结果

## 功能模块

### 1. 数据库管理
- 数据库连接和断开
- 数据库创建
- 数据库信息显示

### 2. 表管理
- 表的创建、修改、删除
- 表结构查看和编辑
- 表数据浏览

### 3. 数据操作
- 数据的增删改查
- 数据导入导出
- 数据搜索和过滤

### 4. SQL 操作
- SQL 语句执行
- SQL 语句格式化
- SQL 脚本管理

### 5. 界面功能
- 树形视图显示
- 表格数据显示
- SQL 编辑器
- DDL 查看器

## 技术特点

1. 使用 Qt 框架开发
2. 支持多种数据类型
3. 提供语法高亮
4. 支持 SQL 脚本管理
5. 提供友好的用户界面
6. 支持数据库基本操作
7. 提供数据导入导出功能

## 使用说明

1. 连接数据库
   - 点击"连接"按钮
   - 选择数据库文件
   - 输入连接信息

2. 表操作
   - 在树形视图中选择表
   - 使用右键菜单进行表操作
   - 在数据标签页中查看和编辑数据

3. SQL 操作
   - 在 SQL 标签页中输入 SQL 语句
   - 点击执行按钮或使用快捷键
   - 查看执行结果

4. 数据操作
   - 使用工具栏按钮进行数据操作
   - 在表格中直接编辑数据
   - 使用搜索功能查找数据

## 注意事项

1. 数据库操作
   - 操作前请确保已连接数据库
   - 重要操作前建议备份数据
   - 注意主键约束

2. SQL 执行
   - 注意 SQL 语句的语法
   - 批量操作时注意事务
   - 注意数据类型匹配

3. 界面操作
   - 注意保存修改
   - 使用快捷键提高效率
   - 注意数据验证
