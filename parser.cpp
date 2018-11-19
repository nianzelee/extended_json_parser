#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

// helper function declaration
void   get_keyword     (const string&, string&, vector<double>&);
void   parse_token     (const string&, vector<string>&);
void   expand_repeat   (const vector<string>&, vector<string>&);
size_t find_repeat_end (const vector<string>&, int);
void   expand_uniform  (ofstream &, const vector<string>&);
void   gen_sample      (string&, const vector<double>&);
double randomDouble    (double, double);

// helper function definition
void
get_keyword(const string &line, string &key, vector<double> &parms)
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
   key  = tokens[0];
   for (size_t i = 1; i < tokens.size(); ++i) 
      parms.push_back(atof(tokens[i].c_str()));
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

void
expand_repeat(const vector<string> &all_lines, vector<string> &expand)
{
   for (size_t i = 0; i < all_lines.size(); ++i) {
      string key;
      vector<double> parms;
      get_keyword(all_lines[i], key, parms);
      if ( key.compare("repeat") == 0 ) {
         if (parms.empty()) {
            printf("[ERROR] Missing argument after repeat on line %zd!\n", i+1);
            exit(1);
         }
         int repeat = (int)parms[0];
         size_t head = i+1;
         size_t tail = find_repeat_end(all_lines, head);
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

size_t
find_repeat_end(const vector<string> &all_lines, int head)
{
   for (size_t i = head; i < all_lines.size(); ++i) {
      string key;
      vector<double> parms;
      get_keyword(all_lines[i], key, parms);
      if (key.compare("\\repeat") == 0) return i;
      if (key.compare("repeat") == 0) {
         printf("[ERROR] Nested repeat is not supported ... (from line %d to %zd)\n", head, i+1);
         exit(1);
      }
   }
   printf("[ERROR] Unclosed repeat starting from line %d\n", head+1);
   exit(1);
}

void
expand_uniform(ofstream &out, const vector<string> &expand)
{
   for (size_t i = 0; i < expand.size(); ++i) {
      string line = expand[i];
      if ( line.find("<") == string::npos ) out << line << endl;
      else {
         string key;
         vector<double> parms;
         get_keyword(line, key, parms);
         if ( key.compare("uniform") == 0 ) {
            if (parms.size() != 2) {
               printf("[ERROR] Wrong number of arguments after uniform!\n");
               printf("%s\n", line.c_str());
               exit(1);
            }
            string sample = line;
            gen_sample(sample, parms);
            out << sample << endl;
         }
         else {
            printf("[WARNING] Unknow keyword %s\n", key.c_str());
            out << line << endl;
         }
      }
   }
}

void
gen_sample(string &sample, const vector<double> &parms)
{
   char str_r[128];
   double r = randomDouble(parms[0], parms[1]);
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
   vector<string> expand;
   expand_repeat(all_lines, expand);
   out.open(argv[2]);
   expand_uniform(out, expand);
   out.close();
   return 0;
}
