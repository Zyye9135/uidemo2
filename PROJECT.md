# GNCDB UI 项目文档

## 项目概述
GNCDB UI 是一个基于Qt的数据库管理工具，提供图形化界面来管理GNCDB数据库。该工具支持数据库的创建、表管理、SQL查询执行以及数据可视化等功能。

## 功能特性

### 数据库管理
- 新建数据库
- 打开现有数据库
- 断开数据库连接
- 数据库状态显示
- 数据库文件备份

### 表管理
- 创建新表
- 删除表
- 清空表
- 表结构查看
- 表数据管理（增删改查）
- 表属性编辑

### SQL功能
- SQL编辑器（支持语法高亮）
- SQL执行
- 查询结果显示
- SQL脚本的保存和加载
- SQL格式化
- 执行当前行SQL

### 界面特性
- 树形结构显示数据库表
- 表数据网格显示
- DDL视图
- 多标签页界面
- 可调整的分割视图
- 自定义上下文菜单

### 树形图样式
- 表名显示为黑色
- 属性显示为深灰色
- 选中项背景为蓝色(rgb(64, 64, 255))
- 未选中项背景为浅灰色(#E6E6E6)
- 焦点管理：
  - 获得焦点时显示蓝色背景
  - 失去焦点时显示浅灰色背景
  - 通过"视图"菜单的"树形图"选项可以恢复焦点

## 技术栈
- Qt 6.8.2
- C++
- GNCDB数据库引擎
- CMake构建系统

## 开发环境
- Linux
- GCC
- Qt Creator
- CMake 3.16+

## 构建说明
1. 确保已安装Qt 6.8.2或更高版本
2. 克隆项目代码
3. 使用Qt Creator打开项目或使用CMake构建
4. 构建并运行

## 使用说明
1. 启动程序
2. 通过"文件"菜单创建新数据库或打开现有数据库
3. 使用树形图浏览数据库表
4. 使用SQL编辑器执行查询
5. 使用数据网格管理表数据
6. 通过"对象"菜单进行表操作
7. 使用"视图"菜单切换不同的视图

## 注意事项
- 确保有足够的磁盘空间
- 建议定期备份数据库文件
- 执行危险操作前请确认
- 大型数据库操作可能需要较长时间
- 建议在操作前先测试SQL语句

## 贡献指南
1. Fork项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 许可证
[待定]

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
│   └── lib/           # GNCDB 静态库
├── mainwindow.*       # 主窗口类
├── main.cpp           # 主程序入口
├── SqlHighlighter.*   # Sql语法高亮器
└── CMakeLists.txt     # CMake构建配置
```

## 关键组件

### 1. 数据库管理器 (DBManager)
- 位置：`database/dbmanager.*`
- 功能：
  - 数据库连接管理
  - SQL 查询执行
  - 结果集处理
  - 错误处理

### 2. 主窗口 (MainWindow)
- 位置：`mainwindow.*`
- 功能：
  - 数据库连接界面
  - 表格显示和管理
  - SQL 查询编辑器
  - 数据操作界面
  - 树形图管理
  - 菜单和工具栏
  - 状态栏信息

### 3. 表对话框 (TableDialog)
- 位置：`ui/tabledialog.*`
- 功能：
  - 创建表界面
  - 表结构定义
  - 字段类型设置
  - 主键设置

### 4. SQL高亮器 (SqlHighlighter)
- 功能：
  - SQL语法高亮
  - 主题设置
  - 自定义关键字支持

## 主要功能
1. 数据库连接管理
2. 表的创建、修改和删除
3. 数据的增删改查
4. SQL 查询执行
5. 表结构查看
6. 树形图导航
7. 多视图切换

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
6. SQL高亮通过 SqlHighlighter 类实现
7. 树形图的样式和焦点管理在 MainWindow 类中处理

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

4. 设置树形图样式：
```cpp
tableTree->setStyleSheet(
    "QTreeWidget::item:selected { background-color:rgb(64, 64, 255); color: white; }"
    "QTreeWidget::item { background-color: #E6E6E6; }"
    "QTreeWidget::item:has-children { color: black; }"
    "QTreeWidget::item:!has-children { color: #404040; }"
);
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

3. 检查树形图焦点：
```cpp
connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now) {
    if (old == tableTree && now != tableTree) {
        // 失去焦点的处理
    } else if (now == tableTree) {
        // 获得焦点的处理
    }
});
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

3. 注释规范：
   - 类和方法使用文档注释
   - 复杂逻辑使用行内注释
   - 使用中文注释提高可读性

## 未来计划
1. 添加数据库备份和恢复功能
2. 实现数据库迁移工具
3. 添加数据导入导出功能
4. 优化大数据量处理性能
5. 添加更多数据库类型支持