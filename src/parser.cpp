#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

// helper function declaration
void   writeLines          (const vector<string>&, const char*);
void   exec                (const char* cmd, string&);
// parsing
void   get_keyword         (const string&, string&, vector<string>&);
void   parse_token         (const string&, vector<string>&);
size_t find_block_end      (const vector<string>&, string, size_t);
// generation
void   expand_choice       (const vector<string>&, vector<string>&);
void   expand_repeat       (const vector<string>&, vector<string>&);
void   expand_values       (const vector<string>&, vector<string>&);
void   gen_sample          (string&, const vector<string>&);
double randomDouble        (double, double);
size_t randomSizet         (size_t, size_t);
void   get_constant        (string&, const vector<string>&);
// parsing JSON
void parse_json_lines      (const vector<string>&, json&);
// generating constraints from JSON
void generate_constraints  (vector<string>&, vector<string>&, json&, string);
// checking satisfiability
bool is_sat                (string&);

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
exec( const char* cmd, string &result )
{
   array<char, 128> buf;
   unique_ptr<FILE, decltype( &pclose )> pipe( popen( cmd, "r" ), pclose );
   if ( !pipe ) throw runtime_error("popen() failed!");
   while ( fgets( buf.data(), buf.size(), pipe.get() ) != nullptr )
      result += buf.data();
}

void
get_keyword(const string &line, string &key, vector<string> &parms)
{
   string temp;
   size_t head, tail;
   vector<string> tokens;
   head = line.find_first_of("<");
   tail = line.find_first_of(">");
   if (head == string::npos || tail == string::npos || head > tail) return;
   temp = line.substr(head+1, tail-head-1);
   parse_token(temp, tokens);
   if (tokens.empty()) {
      printf("[ERROR] No keyword is found inside <> ...\n");
      printf("%s\n", line.c_str());
      exit(1);
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
find_block_end(const vector<string> &all_lines, string block, size_t head)
{
   string end = "/" + block;
   for (size_t i = head; i < all_lines.size(); ++i) {
      string key;
      vector<string> parms;
      get_keyword(all_lines[i], key, parms);
      if (key.compare(end) == 0) return i;
      if (key.compare(block) == 0) {
         printf("[ERROR] Nested %s is not supported ... (from line %zd to %zd)\n", block.c_str(), head, i+1);
         exit(1);
      }
   }
   printf("[ERROR] Unclosed %s starting from line %zd\n", block.c_str(), head+1);
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
         if (head == tail) {
            printf("[ERROR] Empty <choice> on line %zd!\n", i);
            printf("%s\n", all_lines[i].c_str());
            exit(1);
         }
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
            printf("[ERROR] Missing argument after <repeat> on line %zd!\n", i+1);
            printf("%s\n", all_lines[i].c_str());
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
expand_values(const vector<string> &all_lines, vector<string> &expand)
{
   for (size_t i = 0; i < all_lines.size(); ++i) {
      string key;
      vector<string> parms;
      get_keyword(all_lines[i], key, parms);
      if ( key.compare("uniform") == 0 ) {
         if (parms.size() < 2) {
            printf("[ERROR] Missing arguments after <uniform>!\n");
            printf("%s\n", all_lines[i].c_str());
            exit(1);
         }
         string sample = all_lines[i];
         gen_sample(sample, parms);
         expand.push_back(sample);
      }
      else if ( key.compare("constant") == 0 ) {
         if (parms.size() < 1) {
            printf("[ERROR] Missing arguments after <constant>!\n");
            printf("%s\n", all_lines[i].c_str());
            exit(1);
         }
         string constant = all_lines[i];
         get_constant(constant, parms);
         expand.push_back(constant);
      }
      else expand.push_back(all_lines[i]);
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

void
get_constant(string &constant, const vector<string> &parms)
{
   size_t head, tail;
   head = constant.find_first_of("<");
   tail = constant.find_first_of(">");
   constant.replace(head, tail-head+1, parms[0]);
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

void
parse_json_lines ( const vector<string> &all_lines, json &object )
{
   string tmp_str;
   char* tmp_c_str;

   for ( size_t i = 0; i < all_lines.size(); ++i ) {
      tmp_str.append( all_lines[ i ]) ;
      tmp_str.append( "\n" );
   }
   tmp_c_str = new char [ tmp_str.length()+1 ];
   strcpy( tmp_c_str, tmp_str.c_str() );

   object = json::parse( tmp_c_str );
}

void
generate_constraints( vector<string> &declarations,
                      vector<string> &constraints,
                      json &object,
                      string prefix )
{
   if ( object.is_object() )
   {
      for ( json::iterator it = object.begin(); it != object.end(); ++it )
      {
         string new_prefix;
         new_prefix.append( prefix );
         if ( ! new_prefix.empty() )
            new_prefix.append( "-" );
         new_prefix.append( it.key() );
         generate_constraints( declarations,
                               constraints,
                               it.value(),
                               new_prefix );
      }
   }
   else if ( object.is_array() )
   {
      for ( size_t i = 0; i < object.size(); ++i )
      {
         string new_prefix;
         new_prefix.append( prefix );
         if ( ! new_prefix.empty() )
            new_prefix.append( "-" );
         new_prefix.append( to_string( i ) );
         generate_constraints( declarations,
                               constraints,
                               object[ i ],
                               new_prefix );
      }
   }
   else if ( object.is_string() )
   {
      string declaration;
      string constraint;
      declaration.append( "(declare-fun " );
      declaration.append( prefix );
      declaration.append( " () Real)" );
      declarations.push_back( declaration );
      constraint.append( "(assert (= " );
      constraint.append( prefix );
      constraint.append( " " );
      constraint.append( object );
      constraint.append( "))" );
      constraints.push_back( constraint );
   }
   else if ( object.is_number() )
   {
      cerr << "Unsupported: number in the JSON file, prefix = "
           << prefix
           << endl;
      exit( 1 );
   }
   else if ( object.is_boolean() )
   {
      cerr << "Unsupported: boolean in the JSON file, prefix = "
           << prefix
           << endl;
      exit( 1 );
   }
}

bool
is_sat ( string &result )
{
  return ( result.front() == 'd' );
}

int
main(int argc, char ** argv)
{
   int samples = 0;
   if ( argc != 3 && argc != 4 ) {
      printf("[INFO] Usage: $> exj_parser <input file> [intermediate file] <output file>\n");
      printf("                 input file:        extended json file\n");
      printf("                 intermediate file: extended json file with <repeat> and <choice> expanded\n");
      printf("                 output file:       regular json file with <uniform> and <constant> expanded\n");
      return 1;
   }
   srand(time(0));
   ifstream in;
   in.open(argv[1]);
   if ( !in.is_open() ) {
      printf("[ERROR] Cannot open input file %s!\n", argv[1]);
      return 1;
   }
   char med[128], out[128];
   if (argc == 3) sprintf(out, "%s", argv[2]);
   if (argc == 4) {
      sprintf(med, "%s", argv[2]);
      sprintf(out, "%s", argv[3]);
   }
   string line;
   vector<string> all_lines;
   while ( getline(in, line) ) all_lines.push_back(line);
   in.close();
   vector<string> no_repeat_lines;
   expand_repeat(all_lines, no_repeat_lines);
   vector<string> no_choice_lines;
   expand_choice(no_repeat_lines, no_choice_lines);
   if (argc == 4) writeLines(no_choice_lines, med);
   while ( samples < 1 )
   {
      vector<string> no_uniform_lines, declarations, constraints,
                     ext_constraints;
      string constraints_filename, cmd, result;
      json object;

      expand_values(no_choice_lines, no_uniform_lines);

      constraints_filename.append( "output.smt2" );
      ext_constraints.push_back( "(assert (< list1-0-variable1 50.0))" );

      parse_json_lines( no_uniform_lines, object );
      declarations.push_back( "(set-logic QF_NRA)" );
      generate_constraints( declarations, constraints, object, "" );
      declarations.insert( declarations.end(),
                           ext_constraints.begin(),
                           ext_constraints.end() );
      declarations.insert( declarations.end(),
                           constraints.begin(),
                           constraints.end() );
      declarations.push_back( "(check-sat)" );
      declarations.push_back( "(exit)" );
      writeLines( declarations, constraints_filename.c_str() );
      cmd.append( "dReal " );
      cmd.append( constraints_filename );
      exec( cmd.c_str(), result );
      if ( is_sat( result ) )
      {
         writeLines(no_uniform_lines, out);
         samples++;
      }
   }
   return 0;
}
