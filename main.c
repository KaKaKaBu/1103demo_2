#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"

// 错误处理宏
#define DB_CHECK(db, rc, msg) \
    if(rc != SQLITE_OK) { \
        fprintf(stderr, "%s: %s\n", msg, sqlite3_errmsg(db)); \
        sqlite3_close(db); \
        return -1; \
    }

// 回调函数，用于查询结果的输出
static int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    printf("%s: ", (const char*)data);

    for(i = 0; i < argc; i++) {
        printf("%s = %s  ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main() {
    sqlite3 *db = NULL;
    char *err_msg = NULL;
    int rc;
    const char *db_name = "embedded_device.db";

    printf("=== SQLite 嵌入式开发示例 ===\n");

    // 1. 打开/创建数据库
    printf("1. 打开数据库...\n");
    rc = sqlite3_open(db_name, &db);
    DB_CHECK(db, rc, "无法打开数据库");
    printf("数据库打开成功: %s\n", db_name);

    // 2. 创建表 - 设备配置表
    printf("\n2. 创建设备配置表...\n");
    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS device_config ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "param_name TEXT NOT NULL UNIQUE,"
        "param_value TEXT,"
        "update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "创表失败: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    printf("设备配置表创建成功\n");

    // 3. 插入配置数据
    printf("\n3. 插入配置数据...\n");
    const char *insert_sql =
        "INSERT OR REPLACE INTO device_config (param_name, param_value) VALUES "
        "('device_name', 'SmartSensor_001'),"
        "('sampling_rate', '1000'),"
        "('wifi_ssid', 'MyNetwork'),"
        "('temperature_threshold', '35.5');";

    rc = sqlite3_exec(db, insert_sql, 0, 0, &err_msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "插入数据失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("配置数据插入成功\n");
    }

    // 4. 查询所有配置
    printf("\n4. 查询设备配置...\n");
    const char *select_sql = "SELECT * FROM device_config;";
    const char *query_title = "设备配置";

    rc = sqlite3_exec(db, select_sql, callback, (void*)query_title, &err_msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "查询失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    // 5. 使用预处理语句更新数据（更安全的方式）
    printf("\n5. 使用预处理语句更新配置...\n");
    sqlite3_stmt *stmt;
    const char *update_sql = "UPDATE device_config SET param_value = ? WHERE param_name = ?;";

    rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    DB_CHECK(db, rc, "预处理语句准备失败");

    // 绑定参数
    sqlite3_bind_text(stmt, 1, "1500", -1, SQLITE_STATIC);  // 新的采样率
    sqlite3_bind_text(stmt, 2, "sampling_rate", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_DONE) {
        printf("采样率更新成功\n");
    } else {
        fprintf(stderr, "更新失败: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    // 6. 查询更新后的结果
    printf("\n6. 查询更新后的配置...\n");
    rc = sqlite3_exec(db, "SELECT param_name, param_value FROM device_config WHERE param_name='sampling_rate';",
                     callback, (void*)"更新结果", &err_msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "查询失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    // 7. 创建传感器数据表并插入示例数据
    printf("\n7. 管理传感器数据...\n");
    const char *create_sensor_table =
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sensor_type TEXT NOT NULL,"
        "value REAL NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    rc = sqlite3_exec(db, create_sensor_table, 0, 0, &err_msg);
    if(rc == SQLITE_OK) {
        printf("传感器数据表创建成功\n");

        // 插入一些示例传感器数据
        const char *insert_sensor_data =
            "INSERT INTO sensor_data (sensor_type, value) VALUES "
            "('temperature', 25.3),"
            "('humidity', 65.2),"
            "('temperature', 26.1),"
            "('pressure', 1013.25);";

        rc = sqlite3_exec(db, insert_sensor_data, 0, 0, &err_msg);
        if(rc == SQLITE_OK) {
            printf("传感器数据插入成功\n");
        }
    }

    // 8. 查询温度数据统计
    printf("\n8. 温度数据统计...\n");
    const char *stats_sql =
        "SELECT "
        "COUNT(*) as count, "
        "AVG(value) as avg_temp, "
        "MAX(value) as max_temp, "
        "MIN(value) as min_temp "
        "FROM sensor_data WHERE sensor_type='temperature';";

    rc = sqlite3_exec(db, stats_sql, callback, (void*)"温度统计", &err_msg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "统计查询失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    // 9. 关闭数据库
    printf("\n9. 关闭数据库...\n");
    sqlite3_close(db);
    printf("数据库已关闭\n");
    printf("=== 示例程序结束 ===\n");

    return 0;
}