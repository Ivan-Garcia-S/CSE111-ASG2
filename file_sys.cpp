// $Id: file_sys.cpp,v 1.7 2019-07-09 14:05:44-07 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>

using namespace std;

#include "debug.h"
#include "file_sys.h"

int inode::next_inode_nr{1};

struct file_type_hash
{
   size_t operator()(file_type type) const
   {
      return static_cast<size_t>(type);
   }
};

ostream &operator<<(ostream &out, file_type type)
{
   static unordered_map<file_type, string, file_type_hash> hash{
       {file_type::PLAIN_TYPE, "PLAIN_TYPE"},
       {file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE"},
   };
   return out << hash[type];
}

inode_state::inode_state()
{
   DEBUGF('i', "root = " << root << ", cwd = " << cwd
                         << ", prompt = \"" << prompt() << "\"");
   root = make_shared<inode>(file_type::DIRECTORY_TYPE);
   root->contents->initializeRoot(root);
   cwd = root;
}

const string &inode_state::prompt() const { return prompt_; }

ostream &operator<<(ostream &out, const inode_state &state)
{
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type) : inode_nr(next_inode_nr++)
{
   this->type = type;
   switch (type)
   {
   case file_type::PLAIN_TYPE:
      contents = make_shared<plain_file>();
      break;
   case file_type::DIRECTORY_TYPE:
      contents = make_shared<directory>();
      break;
   }
   DEBUGF('i', "inode " << inode_nr << ", type = " << type);
}

int inode::get_inode_nr() const
{
   DEBUGF('i', "inode = " << inode_nr);
   return inode_nr;
}

file_error::file_error(const string &what) : runtime_error(what)
{
}

const wordvec &base_file::readfile() const
{
   throw file_error("is a " + error_file_type());
}

void base_file::writefile(const wordvec &)
{
   throw file_error("is a " + error_file_type());
}

void base_file::remove(const string &)
{
   throw file_error("is a " + error_file_type());
}

inode_ptr base_file::mkdir(const string &)
{
   throw file_error("is a " + error_file_type());
}

inode_ptr base_file::mkfile(const string &)
{
   throw file_error("is a " + error_file_type());
}

size_t plain_file::size() const
{
   size_t size{data.size()};
   DEBUGF('i', "size = " << size);
   return size;
}

const wordvec &plain_file::readfile() const
{
   DEBUGF('i', data);
   return data;
}

void plain_file::writefile(const wordvec &words)
{
   DEBUGF('i', words);
   if (words.size() == 0){
      data.push_back("");
      return;
   }
   data.erase(data.begin(), data.end());
   vector<const string>::iterator index;
   for (index = words.begin() + 2; index != words.end(); index++)
   {
      data.push_back(*index);
   }
}

size_t directory::size() const
{
   size_t size{dirents.size()};
   DEBUGF('i', "size = " << size);
   return size;
}

void directory::remove(const string &filename)
{
   DEBUGF('i', filename);
   bool check = false;
   map<string, inode_ptr>::iterator index;
   for (index = dirents.begin(); index != dirents.end(); index++)
   {
      if (index->first == filename)
      {
         file_type type = index->second->getFileType();
         if ( type == file_type::PLAIN_TYPE){
            dirents.erase(filename);
         }
         else{
            if ( dirents.size() > 2 ) {
               throw file_error ( " is not empty.");
            }
            dirents.erase(filename);
         }
         
         check = true;
      }
   }
   if (!check)
   {
      throw file_error("is a " + error_file_type());
   }
}

inode_ptr directory::mkdir(const string &dirname)
{
   DEBUGF('i', dirname);
   const auto result = dirents.find(dirname);

   if (result != dirents.end())
   {
      throw file_error("is a " + error_file_type());
   }
   inode_ptr newDirectoryPtr = make_shared<inode>(file_type::DIRECTORY_TYPE);
   map<string, inode_ptr>::iterator index = dirents.begin();
   index++;
   inode_ptr parent_ptr = index->second;
   newDirectoryPtr->getContents(file_type::DIRECTORY_TYPE)->initializeDirectory(parent_ptr, newDirectoryPtr);
   dirents.insert({dirname, newDirectoryPtr});
   return newDirectoryPtr;
}

inode_ptr directory::mkfile(const string &filename)
{
   DEBUGF('i', filename);
   inode_ptr newFilePtr = make_shared<inode>(file_type::PLAIN_TYPE);
   dirents.insert({filename, newFilePtr});
   return newFilePtr;
}
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Self functions

inode_ptr inode_state::getCwd() {
   return cwd;
}

void inode_state::setCwd(inode_ptr path){
   cwd = path;
}

inode_ptr inode_state::getRoot() {
   return root;
}

void inode_state::setPrompt(const wordvec &words){
   for ( int loopIndex = 0 ; loopIndex < words.size() ; loopIndex++) {
      prompt_ = prompt_ + words[loopIndex];
   }
}

base_file_ptr inode::getContents(file_type fType)
{
   switch (fType)
   {
   case file_type::PLAIN_TYPE:
      return dynamic_pointer_cast<plain_file>(contents);
      break;
   case file_type::DIRECTORY_TYPE:
      return dynamic_pointer_cast<directory>(contents);
      break;
   }
}

void inode_state::printList(inode_ptr currentDir){
   map<string, inode_ptr> printDir = currentDir->getContents(file_type::DIRECTORY_TYPE)->getDirents();
   map<string, inode_ptr>::iterator index;
   for(index = printDir.begin(); index != printDir.end(); index++ ) {
      if (index->first == ".." || index->first == "."){
         cout << "     " << index->second->inode_nr << "       " << index->second->getContents(file_type::DIRECTORY_TYPE)->size() 
         << "  " << index->first << "\n";
      }
      else if (index->second->getFileType() == file_type::PLAIN_TYPE) {
         cout << "     " << index->second->inode_nr << "       " << index->second->getContents(file_type::PLAIN_TYPE)->size() 
         << "  " << index->first << "\n" ;
      }
      else if (index->second->getFileType() == file_type::DIRECTORY_TYPE) {
         cout << "     " << index->second->inode_nr << "       " << index->second->getContents(file_type::PLAIN_TYPE)->size() 
         << "  " << index->first << "/" << "\n" ;
      }
   }
}

void inode_state::printListRecursive(string filename, inode_ptr currentDir){
   if (filename == "." || filename== ".."){
      cout << filename << ":" << "\n" ;
   }
   else{
      cout << "/" << filename << ":" << "\n" ;
   }
   printList(currentDir);
   map<string, inode_ptr> checkDir = currentDir->getContents(file_type::DIRECTORY_TYPE)->getDirents();
   map<string, inode_ptr>::iterator index = checkDir.begin();
   index++; 
   index++;
   while (index != checkDir.end()){
      if (index->second->getFileType() == file_type::DIRECTORY_TYPE){
         printListRecursive(index->first, index->second);
      }
      index++;
   }
}

string inode_state::getPath(inode_ptr current) {
   string path {""};
   if (current == root) {
      path = "/" ;
   }
   else {
      while ( current != root){
         current = current->getContents(file_type::DIRECTORY_TYPE)->getDirents().find("..")->second;
         map<string, inode_ptr> check = current->getContents(file_type::DIRECTORY_TYPE)->getDirents();
         map<string, inode_ptr>::iterator index;
         for(index = check.begin(); index != check.end(); index++ ) {
            if (index->second == current){
               path = "/" + index->first + path ;
            }
         }
      }
   }
   return path;
}

void directory::initializeRoot(inode_ptr root)
{
   dirents.insert({"..", root});
   dirents.insert({".", root});
}

void directory::initializeDirectory(inode_ptr parent, inode_ptr current)
{
   dirents.insert({"..", parent});
   dirents.insert({".", current});
}

map<string, inode_ptr> plain_file::getDirents()
{
   throw file_error(" is a " + error_file_type());
}

map<string, inode_ptr> directory::getDirents()
{
   return dirents;
}

file_type inode::getFileType() {
   return type;
}

wordvec directory::getData() {
   throw file_error("is a " + error_file_type());
}

wordvec plain_file::getData() {
   return data;
}
