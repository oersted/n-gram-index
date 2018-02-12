#include <unordered_map>
#include <utility>
#include <set>
#include <memory>
#include <algorithm>
#include <iterator>
#include <functional>
#include <vector>
#include <cstddef>
#include <iostream>
#include <string>
#include <sstream>

#define SPACER '*'
#define PADDING '$'

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

using namespace std;

namespace Impl {

    template<class Doc, class Consumer>
    struct n_gram
	{
		typedef std::pair<const char*, unsigned int> CStrLen;
		typedef std::vector<CStrLen> CStrLenList;

		// Keys
		typedef unsigned long DocKey;
		typedef unsigned long DocHash;
		typedef unsigned long NgramHash;

		// N-gram inverted index
		typedef std::pair<DocKey, CStrLen> IndexedDoc;
		typedef std::list<IndexedDoc> IndexedDocList;
		typedef std::set<IndexedDoc> IndexedDocSet;
		typedef std::unordered_map<NgramHash, IndexedDocSet> NgramInvertedIndex;

		// Document index
		typedef std::unordered_map<DocKey, CStrLen> DocIndex;

		// Document inverted index
		typedef std::unordered_map<DocHash, DocKey> DocInvIndex;

		// Document counters
		typedef std::unordered_map<DocKey, unsigned long> Counter;
		typedef std::pair<DocKey, unsigned long> Count;
		typedef std::vector<Count> CountVector;

		// ============================= UTILS =============================

		inline const NgramHash hash(const char *c_str, const unsigned int len)
		{
			unsigned long h = 5381;
			for (unsigned int i = 0; i < len; i++)
				h = ((h << 5) + h) + c_str[i];
			return h;
		}

        struct CountByValueAsc {
          bool operator() (const Count& a, const Count& b) const {
            return a.second < b.second;
          };
        };

		struct CountByValueDesc {
          bool operator() (const Count& a, const Count& b) const {
            return a.second > b.second;
          };
        };

		// ============================= ===== =============================

		Consumer &consumer_;
		const unsigned int n;
		NgramInvertedIndex inverted_index;
		DocIndex doc_index;
        string padding;

        n_gram(unsigned int n, Consumer &consumer) : n(n), consumer_(consumer) {
            padding = string(n - 1, PADDING);
        }

        ~n_gram()
		{
			for (IndexedDoc indexed_doc: doc_index) {
				Doc *doc = indexed_doc.second;
				consumer_.decr_refs(doc);
			}
		}

		const int size()
		{
			return doc_index.size();
		}

		void add_line(DocKey key, Doc *doc)
		{
			auto found = doc_index.find(key);
			if (found != doc_index.end())
				return;

			consumer_.incr_refs(doc);

			doc_index.insert({key, doc});
			add_del_index(key, doc);
		}

		void add_del_index(DocKey key, Doc *python_doc)
		{
			char *doc_cstr;
			const unsigned int doc_size = consumer_.get_c_string(python_doc, doc_cstr);
			CStrLen doc = {doc_cstr, doc_size};

            // Padding and spacing
            string doc_str(doc_cstr);
            string processed_str;
            pad_and_space(doc_str, processed_str);
            const char *processed_cstr = processed_str.c_str();
            const unsigned int processed_size = processed_str.size();
            CStrLen processed = {processed_cstr, processed_size};

			CStrLenList ngrams;
			create_ngrams(ngrams, cstr, size);

			IndexedDoc indexed_doc = std::make_pair(key, processed);

			for (CStrLen &ngram : ngrams) {
				NgramHash h = hash(ngram.first, ngram.second);
				auto found = inverted_index.find(h);
				if (found == inverted_index.end()) {
					if (is_add) {
						IndexedDocSet new_doc_set;
						new_doc_set.insert(indexed_doc);
						inverted_index.insert(std::make_pair(h, new_doc_set));
					}
				} else {
					IndexedDocSet &old_doc_set = found->second;

					if (is_add) {
						old_doc_set.insert(indexed_doc);
					} else {
						old_doc_set.erase(indexed_doc);
						if (old_doc_set.empty()) {
							inverted_index.erase(h);
						}
					}
				}
			}
		}
		
		Count search_one(Doc *query, const unsigned int max_edit_dist) {
		    // TODO
		}

		CountVector search_all(Doc *query, const unsigned int max_edit_dist)
		{
			char *query_cstr;
			const int query_size = consumer_.get_c_string(query, query_cstr);

            // Padding and spacing
            string query_str(query_cstr);
            string processed;
            pad_and_space(query_str, processed);
            const char *processed_cstr = processed.c_str();
            const unsigned int processed_size = processed.size();

			CStrLenList ngrams;
			create_ngrams(ngrams, processed_cstr, processed_size);

			CountVector result;

			// Match none if empty string
			if (ngrams.empty()) {
				return result;
			}

            // Find results
            Counter counter;
			for (CStrLen &ngram : ngrams) {

                // Query the ngram inverted index
				NgramHash key = hash(ngram.first, ngram.second);
				auto found = inverted_index.find(key);
				if (found == inverted_index.end()) continue;
				IndexedDocSet &resultSet = found->second;

                // Accumulate found results in counter
				for (IndexedDoc indexed_doc : found->second) {
				    ++counter[indexed_doc.first];
				}
			}

			// Select valid results
            CountVector edit_distances;
			for (Count count : counter) {
				DocKey key = count.first;
				unsigned long ncount = count.second;

                auto found = doc_index.find(key);
                Doc *doc = found->second;
                char *doc_orig_cstr;
    			const unsigned int doc_orig_size = consumer_.get_c_string(doc, doc_orig_cstr);

    			// Padding and spacing
    			string doc_orig_str(doc_orig_cstr);
                string doc_str;
                pad_and_space(doc_orig_str, doc_str);
                const char *doc_cstr = doc_str.c_str();
                const unsigned int doc_size = doc_str.size();

                // Choose lower bound of ngram matches
                unsigned int min_ngrams = max(processed.size(), doc_str.size()) - n + 1;
                if (min_ngrams > max_edit_dist * n) {
                    min_ngrams -= max_edit_dist * n;
                } else {
                    min_ngrams = 0;
                }

                // Check edit distance
			    if (ncount < min_ngrams) continue;
                const unsigned int ed = edit_distance(processed_cstr, doc_cstr);
                if (ed <= max_edit_dist) {
                    edit_distances.push_back({key, ed});
                }
			}

            // Sort again by edit distance
            sort(edit_distances.begin(), edit_distances.end(), CountByValueAsc());

			return edit_distances;
		}

        void pad_and_space(string old_str, string &new_str) {
            stringstream new_stream;
            new_stream << padding;

            int pos = -1;
            int prev_pos = -1;
            while (true) {
                pos = old_str.find(SPACER, pos + 1);

                if (pos == string::npos) {
                    new_stream << old_str.substr(prev_pos + 1, old_str.size());
                    new_stream << padding;
                    break;
                } else {
                    new_stream << old_str.substr(prev_pos + 1, pos - prev_pos - 1);
                    new_stream << padding;
                }

                prev_pos = pos;
            }
            
            new_str = new_stream.str();
        }

		void create_ngrams(CStrLenList &ngrams, const char *c_str, const unsigned int size)
		{
			if (size < n) return;
			unsigned int pos = 0;
			for (unsigned int pos = 0; pos < size - n; pos++) {
				ngrams.push_back({&c_str[pos], n});
			}
		}

        const unsigned int edit_distance(const char *s1, const char *s2) {
            unsigned int s1len, s2len, x, y, lastdiag, olddiag;
            s1len = strlen(s1);
            s2len = strlen(s2);
            unsigned int column[s1len+1];
            for (y = 1; y <= s1len; y++)
                column[y] = y;
            for (x = 1; x <= s2len; x++) {
                column[0] = x;
                for (y = 1, lastdiag = x-1; y <= s1len; y++) {
                    olddiag = column[y];
                    column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
                    lastdiag = olddiag;
                }
            }
            return(column[s1len]);
        }
	};
}
