//
// Created by AWAY on 26-1-25.
//
#include <iostream>
#include <mysqlx/xdevapi.h>

int main() {
    std::cout << "Starting MySQL X DevAPI Test..." << std::endl;

    try {
        // X DevAPI uses a connection string or SessionSettings
        // Default X Protocol port is usually 33060
        std::cout << "Connecting to tcp://127.0.0.1:33060 ..." << std::endl;
        
        // Note: Change port to 33060 if using X Protocol, or check if your server supports it.
        // If your server only supports classic protocol, X DevAPI might not work easily without configuration.
        mysqlx::Session sess("127.0.0.1", 33060, "root", "caleb759268873");

        std::cout << "Connected successfully!" << std::endl;
        
        // Create a schema object
        // mysqlx::Schema db = sess.getSchema("test_db");
        
        // Example: execute a simple SQL query
        mysqlx::SqlResult result = sess.sql("SELECT 1").execute();
        mysqlx::Row row = result.fetchOne();
        std::cout << "Query Result: " << row[0] << std::endl;

        sess.close();

    } catch (const mysqlx::Error &err) {
        std::cerr << "MySQL Error: " << err << std::endl;
        return 1;
    } catch (std::exception &ex) {
        std::cerr << "STD Exception: " << ex.what() << std::endl;
        return 1;
    } catch (const char *ex) {
        std::cerr << "Exception: " << ex << std::endl;
        return 1;
    }

    return 0;
}