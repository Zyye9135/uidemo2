# GNCDB UI 项目文档

## 项目概述
GNCDB UI 是一个基于 Qt6 的数据库管理工具，用于与 GNCDB 数据库进行交互。项目使用 C++ 开发，采用 CMake 构建系统。

## 技术栈
- C++17
- Qt 6.8.2
- CMake 3.16+
- GNCDB 数据库库

## 项目结构
```
gncdb_ui2/
├── database/           # 数据库交互模块
│   ├── dbmanager.*    # 数据库管理器类
│   └── sqlresult.h    # SQL 查询结果结构定义
├── ui/                # 用户界面模块
│   └── tabledialog.*  # 表操作对话框
├── gncdblib/          # GNCDB 核心库
│   ├── include/       # GNCDB 头文件
│   └── lib/          # GNCDB 静态库
└── main.*            # 主程序入口
```

## 关键组件

### 1. 数据库管理器 (DBManager)
- 位置：`database/dbmanager.*`
- 功能：
  - 数据库连接管理
  - SQL 查询执行
  - 结果集处理

### 2. 主窗口 (MainWindow)
- 位置：`mainwindow.*`
- 功能：
  - 数据库连接界面
  - 表格显示和管理
  - SQL 查询编辑器
  - 数据操作界面

### 3. 表对话框 (TableDialog)
- 位置：`ui/tabledialog.*`
- 功能：
  - 创建表界面
  - 表结构定义
  - 字段类型设置

## 主要功能
1. 数据库连接管理
2. 表的创建、修改和删除
3. 数据的增删改查
4. SQL 查询执行
5. 表结构查看

## 开发环境设置
1. 安装依赖：
   ```bash
   # Qt6 开发环境
   sudo apt install qt6-base-dev
   
   # CMake 构建工具
   sudo apt install cmake
   ```

2. 构建步骤：
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

## 提示 AI 助手
在处理此项目时，请注意：
1. 项目使用 CMake 构建系统，主要配置在根目录的 CMakeLists.txt 中
2. 使用 Qt6 的信号槽机制进行界面交互
3. 数据库操作通过 DBManager 类进行封装
4. 主要的用户界面逻辑在 MainWindow 类中实现
5. 表操作相关的界面在 TableDialog 类中实现

## 常见任务代码示例
1. 创建新的数据库连接：
```cpp
DBManager* dbManager = new DBManager();
bool success = dbManager->open("path/to/database.db");
```

2. 执行 SQL 查询：
```cpp
SQLResult result;
dbManager->executeQuery("SELECT * FROM tableName", &result);
```

3. 添加新的用户界面组件：
```cpp
// 在 MainWindow 类中
void MainWindow::addNewComponent() {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    // 添加更多组件...
}
```

## 调试提示
1. 使用 QDebug 输出调试信息：
```cpp
qDebug() << "调试信息";
```

2. 检查数据库错误：
```cpp
if (!dbManager->isOpen()) {
    qDebug() << "数据库连接失败";
}
```

## 项目约定
1. 命名规范：
   - 类名使用大驼峰：`MainWindow`, `DBManager`
   - 变量名使用小驼峰：`dbManager`, `tableNames`
   - 私有成员变量前缀使用下划线：`_dbManager`

2. 代码组织：
   - 界面相关代码放在 ui/ 目录
   - 数据库相关代码放在 database/ 目录
   - 共用的工具类和函数放在 utils/ 目录（如果需要）