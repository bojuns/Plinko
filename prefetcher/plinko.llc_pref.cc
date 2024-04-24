/* Bingo */

#include "cache.h"
#include <bits/stdc++.h>

using namespace std;

class Table {
  public:
    Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

    void set_row(int row, const vector<string> &data, int start_col = 0) {
        assert(data.size() + start_col == this->width);
        for (unsigned col = start_col; col < this->width; col += 1)
            this->set_cell(row, col, data[col]);
    }

    void set_col(int col, const vector<string> &data, int start_row = 0) {
        assert(data.size() + start_row == this->height);
        for (unsigned row = start_row; row < this->height; row += 1)
            this->set_cell(row, col, data[row]);
    }

    void set_cell(int row, int col, string data) {
        assert(0 <= row && row < (int)this->height);
        assert(0 <= col && col < (int)this->width);
        this->cells[row][col] = data;
    }

    void set_cell(int row, int col, double data) {
        this->oss.str("");
        this->oss << setw(11) << fixed << setprecision(8) << data;
        this->set_cell(row, col, this->oss.str());
    }

    void set_cell(int row, int col, int64_t data) {
        this->oss.str("");
        this->oss << setw(11) << std::left << data;
        this->set_cell(row, col, this->oss.str());
    }

    void set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

    void set_cell(int row, int col, uint64_t data) { this->set_cell(row, col, (int64_t)data); }

    string to_string() {
        vector<int> widths;
        for (unsigned i = 0; i < this->width; i += 1) {
            int max_width = 0;
            for (unsigned j = 0; j < this->height; j += 1)
                max_width = max(max_width, (int)this->cells[j][i].size());
            widths.push_back(max_width + 2);
        }
        string out;
        out += Table::top_line(widths);
        out += this->data_row(0, widths);
        for (unsigned i = 1; i < this->height; i += 1) {
            out += Table::mid_line(widths);
            out += this->data_row(i, widths);
        }
        out += Table::bot_line(widths);
        return out;
    }

    string data_row(int row, const vector<int> &widths) {
        string out;
        for (unsigned i = 0; i < this->width; i += 1) {
            string data = this->cells[row][i];
            data.resize(widths[i] - 2, ' ');
            out += " | " + data;
        }
        out += " |\n";
        return out;
    }

    static string top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

    static string mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

    static string bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

    static string line(const vector<int> &widths, string left, string mid, string right) {
        string out = " " + left;
        for (unsigned i = 0; i < widths.size(); i += 1) {
            int w = widths[i];
            for (int j = 0; j < w; j += 1)
                out += "─";
            if (i != widths.size() - 1)
                out += mid;
            else
                out += right;
        }
        return out + "\n";
    }

  private:
    unsigned width;
    unsigned height;
    vector<vector<string>> cells;
    ostringstream oss;
};

template <class T> class InfiniteCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        if (!entry)
            return nullptr;
        entry->valid = false;
        this->last_erased_entry = *entry;
        int num_erased = this->entries.erase(key);
        assert(num_erased == 1);
        return &this->last_erased_entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        entries[key] = {key, 0, key, true, data};
        return {};
    }

    Entry *find(uint64_t key) {
        auto it = this->entries.find(key);
        if (it == this->entries.end())
            return nullptr;
        Entry &entry = (*it).second;
        assert(entry.tag == key && entry.valid);
        return &entry;
    }

    /**
     * For debugging purposes.
     */
    string log(vector<string> headers, function<void(Entry &, Table &, int)> write_data) {
        Table table(headers.size(), entries.size() + 1);
        table.set_row(0, headers);
        unsigned i = 0;
        for (auto &x : this->entries)
            write_data(x.second, table, ++i);
        return table.to_string();
    }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

  protected:
    Entry last_erased_entry;
    unordered_map<uint64_t, Entry> entries;
    int debug_level = 0;
};

template <class T> class SetAssociativeCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    SetAssociativeCache(int size, int num_ways)
        : size(size), num_ways(num_ways), num_sets(size / num_ways), entries(num_sets, vector<Entry>(num_ways)),
          cams(num_sets) {
        assert(size % num_ways == 0);
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                entries[i][j].valid = false;
    }

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        int num_erased = cam.erase(tag);
        if (entry)
            entry->valid = false;
        assert(entry ? num_erased == 1 : num_erased == 0);
        return entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        vector<Entry> &set = this->entries[index];
        int victim_way = -1;
        for (int i = 0; i < this->num_ways; i += 1)
            if (!set[i].valid) {
                victim_way = i;
                break;
            }
        if (victim_way == -1) {
            victim_way = this->select_victim(index);
        }
        Entry &victim = set[victim_way];
        Entry old_entry = victim;
        victim = {key, index, tag, true, data};
        auto &cam = cams[index];
        if (old_entry.valid) {
            int num_erased = cam.erase(old_entry.tag);
            assert(num_erased == 1);
        }
        cam[tag] = victim_way;
        return old_entry;
    }

    Entry *find(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        if (cam.find(tag) == cam.end())
            return nullptr;
        int way = cam[tag];
        Entry &entry = this->entries[index][way];
        assert(entry.tag == tag && entry.valid);
        return &entry;
    }

    /**
     * For debugging purposes.
     */
    string log(vector<string> headers, function<void(Entry &, Table &, int)> write_data) {
        vector<Entry> valid_entries = this->get_valid_entries();
        Table table(headers.size(), valid_entries.size() + 1);
        table.set_row(0, headers);
        for (unsigned i = 0; i < valid_entries.size(); i += 1)
            write_data(valid_entries[i], table, i + 1);
        return table.to_string();
    }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

  protected:
    /**
     * @return The way of the selected victim.
     */
    virtual int select_victim(uint64_t index) {
        /* random eviction policy if not overriden */
        return rand() % this->num_ways;
    }

    vector<Entry> get_valid_entries() {
        vector<Entry> valid_entries;
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                if (entries[i][j].valid)
                    valid_entries.push_back(entries[i][j]);
        return valid_entries;
    }

    int size;
    int num_ways;
    int num_sets;
    vector<vector<Entry>> entries;
    vector<unordered_map<uint64_t, int>> cams;
    int debug_level = 0;
};

template <class T> class LRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    LRUSetAssociativeCache(int size, int num_ways)
        : Super(size, num_ways), lru(this->num_sets, vector<uint64_t>(num_ways)) {}

    void set_mru(uint64_t key) { *this->get_lru(key) = this->t++; }

    void set_lru(uint64_t key) { *this->get_lru(key) = 0; }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        vector<uint64_t> &lru_set = this->lru[index];
        return min_element(lru_set.begin(), lru_set.end()) - lru_set.begin();
    }

    uint64_t *get_lru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        return &this->lru[index][way];
    }

    vector<vector<uint64_t>> lru;
    uint64_t t = 1;
};

template <class T> class NMRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    NMRUSetAssociativeCache(int size, int num_ways) : Super(size, num_ways), mru(this->num_sets) {}

    void set_mru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        this->mru[index] = way;
    }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        int way = rand() % (this->num_ways - 1);
        if (way >= mru[index])
            way += 1;
        return way;
    }

    vector<int> mru;
};

template <class T> class LRUFullyAssociativeCache : public LRUSetAssociativeCache<T> {
    typedef LRUSetAssociativeCache<T> Super;

  public:
    LRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class NMRUFullyAssociativeCache : public NMRUSetAssociativeCache<T> {
    typedef NMRUSetAssociativeCache<T> Super;

  public:
    NMRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class DirectMappedCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    DirectMappedCache(int size) : Super(size, 1) {}
};

class FilterTableData {
  public:
    // Actual data
    uint64_t region;
    uint64_t pc;
    int offset;

    // Constructor
    FilterTableData(uint64_t region, uint64_t pc, int offset) : region(region), pc(pc), offset(offset) {}
    // Copy Constructor
    FilterTableData(const FilterTableData *other) : region(other->region), pc(other->pc), offset(other->offset) {}

    // Overload comparison operator to check half size region around entry
    bool matches(uint64_t other_region, int other_offset, int size) {
        return get_abs_diff(other_region, other_offset, size) <= size / 2;
    }

    uint64_t get_abs_diff(uint64_t other_region, int other_offset, int size) {
        uint64_t other_addr = other_region * size + other_offset;
        uint64_t my_addr = this->region * size + this->offset;
        uint64_t abs_diff = other_addr > my_addr ? (other_addr - my_addr) : (my_addr - other_addr);
        return abs_diff;
    }
};

class FilterTable {
  public:
    int size;
    int pattern_len;
    vector<shared_ptr<FilterTableData>> table;
    FilterTable(int size, int pattern_len) : size(size), pattern_len(pattern_len) {
        assert(__builtin_popcount(size) == 1);
        assert(__builtin_popcount(pattern_len) == 1);
    }

    vector<shared_ptr<FilterTableData>> find(uint64_t region_number, int offset) {
        vector<shared_ptr<FilterTableData>> found;
        // Search for matching values
        auto iter = table.begin();
        while(iter != table.end()) {
            if((*iter)->matches(region_number, offset, this->pattern_len)) {
                found.push_back(*iter);
                iter = table.erase(iter);
            } else {
                iter++;
            }
        }
        // No match found
        if (found.size() == 0)
            return found;
        // Found a matches, make LRU
        for (auto match : found) {
            assert(match->matches(region_number, offset, this->pattern_len));
            table.insert(table.begin(), match);
        }
        return found;
    }

    void insert(uint64_t region_number, uint64_t pc, int offset) {
        assert(this->find(region_number, offset).size() == 0);
        shared_ptr<FilterTableData> new_ptr(new FilterTableData(region_number, pc, offset));
        table.insert(table.begin(), new_ptr);
        // Removing LRU entry
        if (table.size() >= this->size) {
            // Free memory and remove LRU entry
            table.pop_back();
        }
    }

    void remove(uint64_t region_number, int offset) {
        auto iter = table.begin();
        while(iter != table.end()) {
            if((*iter)->matches(region_number, offset, this->pattern_len)) {
                iter = table.erase(iter);
            } else {
                iter++;
            }
        }
    }
};

class AccumulationTableData {
  public:
    uint64_t pc;
    uint64_t region;
    int offset;
    vector<int> pattern;
    // Constructor
    AccumulationTableData(uint64_t pc, uint64_t region, int offset, vector<int> pattern) :
        pc(pc), region(region), offset(offset), pattern(pattern) {};
    
    // Copy Constructor
    AccumulationTableData(const AccumulationTableData *other) : 
        pc(other->pc), region(other->region), offset(other->offset), pattern(other->pattern) {};
     
    // Overload comparison operator to check half size region around entry
    bool matches(uint64_t other_region, int other_offset, int size) {
        // return other_region == this->region;
        return get_abs_diff(other_region, other_offset, size) <= size / 2;
    }

    uint64_t get_abs_diff(uint64_t other_region, int other_offset, int size) {
        int64_t other_addr = other_region * size + other_offset;
        int64_t my_addr = this->region * size + this->offset;
        int64_t abs_diff = labs(other_addr - my_addr);
        return abs_diff;
    }
    uint64_t get_diff(uint64_t other_region, int other_offset, int size) {
        int64_t other_addr = other_region * size + other_offset;
        int64_t my_addr = this->region * size + this->offset;
        int64_t diff = other_addr - my_addr;
        assert(diff >= -size && diff <= size);
        return diff;
    }
    
};

class AccumulationTable {
  public:
    vector<shared_ptr<AccumulationTableData>> table;
    AccumulationTable(int size, int pattern_len) : size(size), pattern_len(pattern_len) {
        assert(__builtin_popcount(size) == 1);
        assert(__builtin_popcount(pattern_len) == 1);
    }

    /**
     * @return A return value of false means that the tag wasn't found in the table and true means success.
     */
    bool set_pattern(uint64_t region_number, int offset) {
        vector<shared_ptr<AccumulationTableData>> found;
        // Search for matching values
        auto iter = table.begin();
        while(iter != table.end()) {
            if((*iter)->matches(region_number, offset, this->pattern_len)) {
                found.push_back(*iter);
                iter = table.erase(iter);
            } else {
                iter++;
            }
        }
        // No match found
        if (found.size() == 0)
            return false;
        // Found a matches, make LRU
        for (auto match : found) {
            match->pattern.push_back(match->get_diff(region_number, offset, this->pattern_len));
            table.insert(table.begin(), match);
        }
        return true;
    }

    shared_ptr<AccumulationTableData> insert(shared_ptr<FilterTableData> data) {
        vector<int> pattern;
        shared_ptr<AccumulationTableData> new_data(new AccumulationTableData(data->pc, data->region, data->offset, pattern));
        new_data->pattern.push_back(new_data->get_diff(data->region, data->offset, this->size));
        table.insert(table.begin(), new_data);
        // Removing LRU entry
        shared_ptr<AccumulationTableData> removed = nullptr;
        if (table.size() >= size) {
            // Remove LRU entry
            removed = table.back();
            table.pop_back();
        }
        return removed;
    }
    vector<shared_ptr<AccumulationTableData>> remove(uint64_t region, int offset) {
        vector<shared_ptr<AccumulationTableData>> removed;

        // Remove elements
        auto iter = table.begin();
        while(iter != table.end()) {
            if((*iter)->matches(region, offset, this->pattern_len)) {
                removed.push_back(*iter);
                iter = table.erase(iter);
            } else {
                iter++;
            }
        }
        return removed;
    }
  private:
    int size;
    int pattern_len;
};

class PrefetchQualityTableData {
    public:
        bool isUseful;
};

class PrefetchQualityTable : public LRUFullyAssociativeCache<PrefetchQualityTableData> {
    typedef LRUFullyAssociativeCache<PrefetchQualityTableData> Super;
    public:
    PrefetchQualityTable(int size) : Super(size) {
        assert(__builtin_popcount(size) == 1);
    } 
    Entry insert(int region_number, bool useful) {
        Entry old_entry = Super::insert(region_number, {useful});
        return old_entry;
    }  
};

enum Event { PC_ADDRESS = 0, PC_OFFSET = 1 };

template <class T> vector<T> my_rotate(const vector<T> &x, int n) {
    vector<T> y;
    int len = x.size();
    n = n % len;
    for (int i = 0; i < len; i += 1)
        y.push_back(x[(i - n + len) % len]);
    return y;
}

#define THRESH 0.30

class PatternHistoryTableData {
    public:
        vector<int> pattern;
};

class PatternHistoryTable : LRUSetAssociativeCache<PatternHistoryTableData> {
    typedef LRUSetAssociativeCache<PatternHistoryTableData> Super;

  public:
    PatternHistoryTable(
        int size, int pattern_len, int min_addr_width, int max_addr_width, int pc_width, int num_ways = 16)
        : Super(size, num_ways), pattern_len(pattern_len), min_addr_width(min_addr_width),
          max_addr_width(max_addr_width), pc_width(pc_width) {
        assert(this->pc_width >= 0);
        assert(this->min_addr_width >= 0);
        assert(this->max_addr_width >= 0);
        assert(this->max_addr_width >= this->min_addr_width);
        assert(this->pc_width + this->min_addr_width > 0);
        assert(__builtin_popcount(pattern_len) == 1);
        this->index_len = __builtin_ctz(this->num_sets);
    }

    /* address is actually block number */
    void insert(uint64_t pc, uint64_t address, vector<int> pattern) {
        uint64_t key = this->build_key(pc, address);
        Entry victim = Super::insert(key, {pattern});
        this->set_mru(key);
    }

    /**
     * @return An un-rotated pattern if match was found, otherwise an empty vector.
     * Finds best match and in case of ties, uses the MRU entry.
     */
    vector<int> find(uint64_t pc, uint64_t address) {
        uint64_t key = this->build_key(pc, address);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &set = this->entries[index];
        // PC + Address mask
        uint64_t min_tag_mask = (1 << (this->pc_width + this->min_addr_width - this->index_len)) - 1;
        // PC + Offset mask
        uint64_t max_tag_mask = (1 << (this->pc_width + this->max_addr_width - this->index_len)) - 1;
        // Keeping track of the footprints for voting
        vector<vector<int>> min_matches;
        vector<int> pattern;
        // Checking all the associative tables
        for (int i = 0; i < this->num_ways; i += 1) {
            // Ignore invalid sets
            if (!set[i].valid)
                continue;
            // Matching the tag for the short event (PC + Offset)
            bool min_match = ((set[i].tag & min_tag_mask) == (tag & min_tag_mask));
            // Matching for the long event (PC + Address)
            bool max_match = ((set[i].tag & max_tag_mask) == (tag & max_tag_mask));
            vector<int> &cur_pattern = set[i].data.pattern;
            // Check for match in long event first
            if (max_match) {
                this->set_mru(set[i].key);
                pattern = cur_pattern;
                break;
            }
            // If no matches in long event check for short event match
            if (min_match) {
                min_matches.push_back(cur_pattern);
            }
        }
        this->last_event = PC_ADDRESS;
        if (pattern.empty()) {
            /* no max match was found, time for a vote! */
            pattern = this->vote(min_matches);
            this->last_event = PC_OFFSET;
        }
        return pattern;
    }

    Event get_last_event() {
        return this->last_event;
    }

  private:
    uint64_t build_key(uint64_t pc, uint64_t address) {
        pc &= (1 << this->pc_width) - 1;            /* use [pc_width] bits from pc */
        address &= (1 << this->max_addr_width) - 1; /* use [addr_width] bits from address */
        uint64_t offset = address & ((1 << this->min_addr_width) - 1);
        uint64_t base = (address >> this->min_addr_width);
        /* base + pc + offset */
        uint64_t key = (base << (this->pc_width + this->min_addr_width)) | (pc << this->min_addr_width) | offset;
        /* CRC */
        uint64_t tag = ((pc << this->min_addr_width) | offset);
        do {
            tag >>= this->index_len;
            key ^= tag & ((1 << this->index_len) - 1);
        } while (tag > 0);
        return key;
    }

    vector<int> vote(const vector<vector<int>> &x, float thresh = THRESH) {
        int n = x.size();
        vector<int> ret;
        map<int, int> counts;
        for (int way = 0; way < n; way++) {
            for (int addr : x[way]) {
                // Not found in map, add it
                if (counts.find(addr) == counts.end()) {
                    counts[addr] = 1;
                // Increment counter
                } else {
                    counts[addr] += 1;
                }
            }
        }
        for (const auto& elem : counts) {
            // If value exceeds threshold, add it to fetch list
            if (1.0 * elem.second / n >= THRESH) {
                ret.push_back(elem.first);
            }
        }
        return ret;
    }

    int pattern_len, index_len;
    int min_addr_width, max_addr_width, pc_width;
    Event last_event;
};

class Bingo {
  public:
    // Pattern length is the number of cache lines in a region
    Bingo(int pattern_len, int min_addr_width, int max_addr_width, int pc_width, int pattern_history_table_size,
        int filter_table_size, int accumulation_table_size)
        : pattern_len(pattern_len), filter_table(filter_table_size, pattern_len),
          accumulation_table(accumulation_table_size, pattern_len),
          pht(pattern_history_table_size, pattern_len, min_addr_width, max_addr_width, pc_width) {}

    /**
     * @return A vector of block numbers that should be prefetched.
     */
    vector<uint64_t> access(uint64_t block_number, uint64_t pc) {
        static int prevented = 0;
        if (this->debug_level >= 1) {
            cerr << "[Bingo] access(block_number=" << block_number << ", pc=" << pc << ")" << endl;
        }
        uint64_t region_number = block_number / this->pattern_len;
        int region_offset = block_number % this->pattern_len;
        // While regions exist in accumulation table, update history and don't prefetch
        // This is effectively "training" the prefetcher
        bool success = this->accumulation_table.set_pattern(region_number, region_offset);
        if (success) {
            accum_no_prefs++;
            return vector<uint64_t>();
        }
        vector<shared_ptr<FilterTableData>> entries = this->filter_table.find(region_number, region_offset);
        // If miss in the filter table, create new filter table entry
        if (entries.size() == 0) {
            // Trigger access is the act of missing in the filter table
            this->filter_table.insert(region_number, pc, region_offset);
            // If not in filter or accumulation table, check PHT for triggering access
            vector<int> pattern = this->find_in_phts(pc, block_number);
            if (pattern.empty())
                return vector<uint64_t>();
            vector<uint64_t> to_prefetch;
            for (int i = 0; i < pattern.size(); i += 1) {
                uint64_t prefetch_block = region_number * this->pattern_len + pattern[i] + region_offset;
                // If the prefetch target is not in the overpredictions vector prefetch it
                if (find(this->overpredictions.begin(), this->overpredictions.end(), prefetch_block) == this->overpredictions.end())
                    to_prefetch.push_back(prefetch_block);
            }
            static int issued = 0;
            issued += to_prefetch.size();
            return to_prefetch;
        }

        // If hit in filter table, with different triggering access transfer data to accumulation table
        for (auto entry : entries) {
            if (!(entry->offset == region_offset && entry->region == region_number)) {
                // Get evicted entry from the accumulation table if exists
                shared_ptr<AccumulationTableData> victim = this->accumulation_table.insert(entry);
                // Add entry to the accumulation table
                this->accumulation_table.set_pattern(region_number, region_offset);
                // Remove entry in the filter table
                this->filter_table.remove(region_number, region_offset);
                // Check if evicted entry is valid, and if so, move to PHT
                if (victim) {
                    /* move from accumulation table to pattern history table */
                    this->insert_in_phts(victim);
                }
            }
        }
        return vector<uint64_t>();
    }

    void eviction(uint64_t block_number) {
        if (this->debug_level >= 1) {
            cerr << "[Bingo] eviction(block_number=" << block_number << ")" << endl;
        }
        /* end of generation */
        uint64_t region_number = block_number / this->pattern_len;
        int region_offset = block_number % this->pattern_len;
        this->filter_table.remove(region_number, region_offset);
        vector<shared_ptr<AccumulationTableData>> entries = this->accumulation_table.remove(region_number, region_offset);
        for (auto entry : entries) {
            /* move from accumulation table to pattern history table */
            this->insert_in_phts(entry);
        }
    }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

    /* stats */
    void add_prefetch(uint64_t block_number) {
        uint64_t region_number = block_number / this->pattern_len;
        Event ev = this->events[region_number];
        this->prefetch_cnt[ev] += 1;
    }

    void add_cover(uint64_t block_number) {
        uint64_t region_number = block_number / this->pattern_len;
        Event ev = this->events[region_number];
        this->cover_cnt[ev] += 1;
    }

    void add_overpredict(uint64_t block_number) {
        uint64_t region_number = block_number / this->pattern_len;
        Event ev = this->events[region_number];
        int region_offset = block_number % this->pattern_len;
        // Add overprediction to fully associative cache
        vector<uint64_t>::iterator first = find(this->overpredictions.begin(), this->overpredictions.end(), block_number);
        if (first != this->overpredictions.end()) {
            this->overpredictions.erase(first);
        } else if (this->overpredictions.size() == 512) {
            this->overpredictions.erase(this->overpredictions.begin());
        }
        this->overpredictions.push_back(block_number);
        this->overpredict_cnt[ev] += 1;
    }

    void reset_stats() {
        for (int i = 0; i < 2; i += 1) {
            this->prefetch_cnt[i] = 0;
            this->cover_cnt[i] = 0;
            this->overpredict_cnt[i] = 0;
        }
    }

    uint64_t get_prefetch_cnt(Event ev) {
        return this->prefetch_cnt[ev];
    }

    uint64_t get_cover_cnt(Event ev) {
        return this->cover_cnt[ev];
    }

    uint64_t get_overpredict_cnt(Event ev) {
        return this->overpredict_cnt[ev];
    }
    uint64_t get_accum_no_prefs() {
        return this->accum_no_prefs;
    }

  private:
    vector<int> find_in_phts(uint64_t pc, uint64_t address) {
        if (this->debug_level >= 1) {
            cerr << "[Bingo] find_in_phts(pc=" << pc << ", address=" << address << ")" << endl;
        }
        vector<int> pattern = this->pht.find(pc, address);
        uint64_t region_number = address / this->pattern_len;
        events[region_number] = this->pht.get_last_event();
        return pattern;
    }

    void insert_in_phts(shared_ptr<AccumulationTableData> entry) {
        static int counter = 0;
        if (this->debug_level >= 1) {
            cerr << "[Bingo] insert_in_phts(...)" << endl;
        }
        uint64_t pc = entry->pc;
        uint64_t address = entry->region * this->pattern_len + entry->offset;
        vector<int> pattern(entry->pattern);
        this->pht.insert(pc, address, pattern);
    }

    int pattern_len;
    FilterTable filter_table;
    AccumulationTable accumulation_table;
    PatternHistoryTable pht;
    int debug_level = 0;
    vector<uint64_t> overpredictions;

    /* stats */
    unordered_map<uint64_t, Event> events;
    uint64_t accum_no_prefs = 0;
    uint64_t prefetch_cnt[2] = {0};
    uint64_t cover_cnt[2] = {0};
    uint64_t overpredict_cnt[2] = {0};
};

/* Bingo settings */
const int REGION_SIZE = 2 * 1024;
const int MIN_ADDR_WIDTH = 5;
const int MAX_ADDR_WIDTH = 16;
const int PC_WIDTH = 16;
const int PHT_SIZE = 16 * 1024;
const int FT_SIZE = 64;
const int AT_SIZE = 128;

vector<Bingo> prefetchers;

/* stats */
unordered_set<uint64_t> prefetched_blocks[NUM_CPUS];

uint64_t roi_prefetch_cnt[NUM_CPUS][2] = {0};
uint64_t roi_cover_cnt[NUM_CPUS][2] = {0};
uint64_t roi_overpredict_cnt[NUM_CPUS][2] = {0};
uint64_t roi_accum_no_prefs[NUM_CPUS] = {0};

void CACHE::llc_prefetcher_initialize_(uint32_t cpu) {
    if (cpu != 0)
        return;

    /* create prefetcher for all cores */
    assert(PAGE_SIZE % REGION_SIZE == 0);
    prefetchers = vector<Bingo>(NUM_CPUS, Bingo(REGION_SIZE >> LOG2_BLOCK_SIZE, MIN_ADDR_WIDTH, MAX_ADDR_WIDTH,
        PC_WIDTH, PHT_SIZE, FT_SIZE, AT_SIZE));
}

void CACHE::llc_prefetcher_operate_(uint32_t cpu, uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type) {
    uint64_t block_number = addr >> LOG2_BLOCK_SIZE;
    
    for (int i = 0; i < NUM_CPUS; i += 1) {
        int cnt = prefetched_blocks[i].erase(block_number);
        if (cnt == 1)
            prefetchers[i].add_cover(block_number);
    }
    // IP is the current PC
    vector<uint64_t> to_prefetch = prefetchers[cpu].access(block_number, ip);
    for (auto &pf_block_number : to_prefetch) {
        // Generating prefetches for the specified blocks from the prefetcher
        uint64_t pf_address = pf_block_number << LOG2_BLOCK_SIZE;
        prefetch_line(cpu, ip, addr, pf_address, FILL_LLC);
    }
}

void CACHE::llc_prefetcher_cache_fill_(uint32_t cpu, uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch,uint64_t evicted_addr) {
    uint64_t block_number = addr >> LOG2_BLOCK_SIZE;
    uint64_t evicted_block = evicted_addr >> LOG2_BLOCK_SIZE;
    
    if (prefetch == 1) {
        prefetched_blocks[cpu].insert(block_number);
        prefetchers[cpu].add_prefetch(block_number);
    }

    for (int i = 0; i < NUM_CPUS; i += 1) {
        int cnt = prefetched_blocks[i].erase(evicted_block);
        if (cnt == 1)
            prefetchers[i].add_overpredict(evicted_block);
    }

    /* inform all bingo modules of the eviction */
    for (int i = 0; i < NUM_CPUS; i += 1)
        prefetchers[i].eviction(evicted_block);
}

void CACHE::llc_prefetcher_inform_warmup_complete_() {
    for (int i = 0; i < NUM_CPUS; i += 1) {
        prefetchers[i].reset_stats();
        prefetched_blocks[i].clear();
    }
}

void CACHE::llc_prefetcher_inform_roi_complete_(uint32_t cpu) {
    for (int i = 0; i < 2; i += 1) {
        roi_prefetch_cnt[cpu][i] = prefetchers[cpu].get_prefetch_cnt(static_cast<Event>(i));
        roi_cover_cnt[cpu][i] = prefetchers[cpu].get_cover_cnt(static_cast<Event>(i));
        roi_overpredict_cnt[cpu][i] = prefetchers[cpu].get_overpredict_cnt(static_cast<Event>(i));
        roi_accum_no_prefs[cpu] = prefetchers[cpu].get_accum_no_prefs();
    }
}

void CACHE::llc_prefetcher_roi_stats_(uint32_t cpu) {
    cout << "* CPU " << cpu << " ROI PC+Address Prefetches: " << roi_prefetch_cnt[cpu][PC_ADDRESS] << endl;
    cout << "* CPU " << cpu << " ROI PC+Offset Prefetches: "  << roi_prefetch_cnt[cpu][PC_OFFSET]  << endl;

    cout << "* CPU " << cpu << " ROI PC+Address Covered Misses: " << roi_cover_cnt[cpu][PC_ADDRESS] << endl;
    cout << "* CPU " << cpu << " ROI PC+Offset Covered Misses: "  << roi_cover_cnt[cpu][PC_OFFSET]  << endl;
    
    cout << "* CPU " << cpu << " ROI PC+Address Overpredictions: " << roi_overpredict_cnt[cpu][PC_ADDRESS] << endl;
    cout << "* CPU " << cpu << " ROI PC+Offset Overpredictions: "  << roi_overpredict_cnt[cpu][PC_OFFSET]  << endl;
 
    cout << "* CPU " << cpu << " ROI Accum Table No Prefetches: "  << roi_accum_no_prefs[cpu] << endl;
}

void CACHE::llc_prefetcher_final_stats_(uint32_t cpu) {
    cout << "* CPU " << cpu << " Total PC+Address Prefetches: " << prefetchers[cpu].get_prefetch_cnt(PC_ADDRESS) << endl;
    cout << "* CPU " << cpu << " Total PC+Offset Prefetches: "  << prefetchers[cpu].get_prefetch_cnt(PC_OFFSET)  << endl;

    cout << "* CPU " << cpu << " Total PC+Address Covered Misses: " << prefetchers[cpu].get_cover_cnt(PC_ADDRESS) << endl;
    cout << "* CPU " << cpu << " Total PC+Offset Covered Misses: "  << prefetchers[cpu].get_cover_cnt(PC_OFFSET)  << endl;
    
    cout << "* CPU " << cpu << " ROI Accum Table No Prefetches: "  << prefetchers[cpu].get_accum_no_prefs() << endl;
}
