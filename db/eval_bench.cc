#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include "leveldb/db.h"
#include "leveldb/options.h"

using namespace std;
using namespace std::chrono;

void benchmark(leveldb::DB* db) {
    const int N = 10000;
    leveldb::WriteOptions write_options;
    leveldb::ReadOptions read_options;

    // 1. Fill data
    cout << "Inserting " << N << " keys..." << endl;
    for (int i = 0; i < N; i++) {
        char buf[20];
        snprintf(buf, sizeof(buf), "key%06d", i);
        db->Put(write_options, buf, "value");
    }

    // 2. Measure Scan
    auto start = high_resolution_clock::now();
    std::vector<std::pair<std::string, std::string>> results;
    db->Scan(read_options, "key001000", "key002000", &results);
    auto end = high_resolution_clock::now();
    auto scan_us = duration_cast<microseconds>(end - start).count();
    cout << "Scan (1000 keys) took: " << scan_us << " us (" << scan_us / 1000.0 << " ms)" << endl;
    cout << "Scan results size: " << results.size() << endl;

    // 3. Measure equivalent Gets
    start = high_resolution_clock::now();
    for (int i = 1000; i < 2000; i++) {
        char buf[20];
        snprintf(buf, sizeof(buf), "key%06d", i);
        string val;
        db->Get(read_options, buf, &val);
    }
    end = high_resolution_clock::now();
    auto gets_us = duration_cast<microseconds>(end - start).count();
    cout << "1000 individual Gets took: " << gets_us << " us (" << gets_us / 1000.0 << " ms)" << endl;

    // 4. Measure DeleteRange
    start = high_resolution_clock::now();
    db->DeleteRange(write_options, "key003000", "key004000");
    end = high_resolution_clock::now();
    auto dr_us = duration_cast<microseconds>(end - start).count();
    cout << "DeleteRange (1000 keys) took: " << dr_us << " us (" << dr_us / 1000.0 << " ms)" << endl;
    
    // 5. Verify Scan after DeleteRange
    results.clear();
    db->Scan(read_options, "key003000", "key004000", &results);
    cout << "Scan after DeleteRange results: " << results.size() << " (expected: 0)" << endl;

    // 6. Measure ForceFullCompaction
    start = high_resolution_clock::now();
    leveldb::Status s = db->ForceFullCompaction();
    end = high_resolution_clock::now();
    auto fc_us = duration_cast<microseconds>(end - start).count();
    cout << "ForceFullCompaction took: " << fc_us << " us (" << fc_us / 1000.0 << " ms)" << endl;
    cout << "ForceFullCompaction status: " << s.ToString() << endl;

    // 7. Verify Scan still correct after compaction
    results.clear();
    db->Scan(read_options, "key003000", "key004000", &results);
    cout << "Scan after compaction results: " << results.size() << " (expected: 0)" << endl;

    // 8. Verify remaining data integrity
    results.clear();
    db->Scan(read_options, "key000000", "key010000", &results);
    cout << "Full scan results: " << results.size() << " (expected: 9000)" << endl;

    // 9. Edge case: start_key > end_key
    results.clear();
    db->Scan(read_options, "key005000", "key001000", &results);
    cout << "Scan (start > end) results: " << results.size() << " (expected: 0)" << endl;

    // 10. Edge case: start_key == end_key
    results.clear();
    db->Scan(read_options, "key005000", "key005000", &results);
    cout << "Scan (start == end) results: " << results.size() << " (expected: 0)" << endl;
}

int main() {
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    
    system("rm -rf /tmp/testdb_eval");
    leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb_eval", &db);
    if (!status.ok()) {
        cerr << "Unable to open/create test database: " << status.ToString() << endl;
        return 1;
    }

    benchmark(db);

    delete db;
    system("rm -rf /tmp/testdb_eval");
    return 0;
}
