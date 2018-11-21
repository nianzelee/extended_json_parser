#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

// helper function declaration
void   writeLines      (const vector<string>&, const char*);
void   get_keyword     (const string&, string&, vector<string>&);
void   parse_token     (const string&, vector<string>&);
size_t find_block_end  (const vector<string>&, string, int);
void   expand_choice   (const vector<string>&, vector<string>&);
void   expand_repeat   (const vector<string>&, vector<string>&);
void   expand_values   (ofstream&, const vector<string>&);
void   gen_sample      (string&, const vector<string>&);
double randomDouble    (double, double);
size_t randomSizet     (size_t, size_t);

// helper function definition
void
writeLines(const vector<string> &lines, const char *name)
{
   ofstream out;
   out.open(name);
   for (size_t i = 0; i < lines.size(); ++i)
      out << lines[i] << endl;
   out.close();
}

void
get_keyword(const string &line, string &key, vector<string> &parms)
{
   string temp;
   size_t head, tail;
   vector<string> tokens;
   head = line.find_first_of("<");
   tail = line.find_first_of(">");
   temp = line.substr(head+1, tail-head-1);
   parse_token(temp, tokens);
   if (tokens.empty()) {
      printf("[WARNING] No keyword is found ...\n");
      return;
   }
   key = tokens[0];
   for (size_t i = 1; i < tokens.size(); ++i) 
      parms.push_back(tokens[i].c_str());
}

void
parse_token(const string &temp, vector<string> &tokens)
{
   size_t head = 0, tail;
   while ((head = temp.find_first_not_of(" ", head)) != string::npos) {
      tail = temp.find_first_of(" ", head);
      tokens.push_back(temp.substr(head, tail-head));
      head = tail;
   }
}

size_t
find_block_end(const vector<string> &all_lines, string block, int head)
{
   string end = "\\" + block;
   for (size_t i = head; i < all_lines.size(); ++i) {
      string key;
      vector<string> parms;
      get_keyword(all_lines[i], key, parms);
      if (key.compare(end) == 0) return i;
      if (key.compare(block) == 0) {
         printf("[ERROR] Nested %s is not supported ... (from line %d to %zd)\n", block.c_str(), head, i+1);
         exit(1);
      }
   }
   printf("[ERROR] Unclosed %s starting from line %d\n", block.c_str(), head+1);
   exit(1);
}

void
expand_choice(const vector<string> &all_lines, vector<string> &expand)
{
   for (size_t i = 0; i < all_lines.size(); ++i) {
      string key;
      vector<string> parms;
      get_keyword(all_lines[i], key, parms);
      if ( key.compare("choice") == 0 ) {
         size_t head   = i+1;
         size_t tail   = find_block_end(all_lines, key, head);
         size_t choice = randomSizet(head, tail-1);
         expand.push_back(all_lines[choice]);
         i = tail;
      }
      else expand.push_back(all_lines[i]);
   }
}

void
expand_repeat(const vector<string> &all_lines, vector<string> &expand)
{
   for (size_t i = 0; i < all_lines.size(); ++i) {
      string key;
      vector<string> parms;
      get_keyword(all_lines[i], key, parms);
      if ( key.compare("repeat") == 0 ) {
         if (parms.empty()) {
            printf("[ERROR] Missing argument after repeat on line %zd!\n", i+1);
            exit(1);
         }
         int repeat = atoi(parms[0].c_str());
         size_t head = i+1;
         size_t tail = find_block_end(all_lines, key, head);
         for (int j = 0; j < repeat; ++j) {
            for ( size_t k = head; k < tail; ++k )
               expand.push_back(all_lines[k]);
            if ( j < repeat-1 ) expand.push_back(",");
         }
         i = tail;
      }
      else expand.push_back(all_lines[i]);
   }
}

void
expand_values(ofstream &out, const vector<string> &expand)
{
   for (size_t i = 0; i < expand.size(); ++i) {
      string line = expand[i];
      if ( line.find("<") == string::npos ) out << line << endl;
      else {
         string key;
         vector<string> parms;
         get_keyword(line, key, parms);
         if ( key.compare("uniform") == 0 ) {
            if (parms.size() < 2) {
               printf("[ERROR] Missing arguments after uniform!\n");
               printf("%s\n", line.c_str());
               exit(1);
            }
            string sample = line;
            gen_sample(sample, parms);
            out << sample << endl;
         }
         else if ( key.compare("constant") == 0 ) {
            if (parms.size() < 1) {
               printf("[ERROR] Missing arguments after constant!\n");
               printf("%s\n", line.c_str());
               exit(1);
            }
            out << parms[0] << endl;
         }
         else {
            printf("[WARNING] Unknow keyword %s\n", key.c_str());
            out << line << endl;
         }
      }
   }
}

void
gen_sample(string &sample, const vector<string> &parms)
{
   char str_r[128];
   double r = randomDouble(atof(parms[0].c_str()), atof(parms[1].c_str()));
   sprintf(str_r, "\"%f\"", r);
   string temp(str_r);
   size_t head, tail;
   head = sample.find_first_of("<");
   tail = sample.find_first_of(">");
   sample.replace(head, tail-head+1, temp);
}

double
randomDouble(double min, double max)
{
   return min + (max-min) * ((double)rand() / (double)RAND_MAX);
}

size_t
randomSizet(size_t min, size_t max)
{
   return min + (size_t)rand() % (max-min+1);
}

int
main(int argc, char ** argv)
{
   if ( argc != 3 ) {
      printf("[INFO] Usage: $> exj_parser <input file> <output file>\n");
      printf("                 input file: extended json file\n");
      printf("                 output file: regular json file\n");
      return 1;
   }
   srand(time(0));
   ifstream in;
   ofstream out;
   in.open(argv[1]);
   if ( !in.is_open() ) {
      printf("[ERROR] Cannot open input file %s!\n", argv[1]);
      return 1;
   }
   string line;
   vector<string> all_lines;
   while ( getline(in, line) ) all_lines.push_back(line);
   in.close();
   vector<string> no_repeat_lines;
   expand_repeat(all_lines, no_repeat_lines);
   vector<string> no_choice_lines;
   expand_choice(no_repeat_lines, no_choice_lines);
   writeLines(no_choice_lines, "temp.json");
   out.open(argv[2]);
   expand_values(out, no_choice_lines);
   out.close();
   return 0;
}
