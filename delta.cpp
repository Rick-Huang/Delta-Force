#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <fstream>
#include <functional>

using namespace std;

const int HASH_SIZE = 90025; // initialize global variables to be used for size of hash table and number of characters per sequence in hash
const int b_seq = 16;

struct bucket // initialize struct that holds each item in the hash table, because it is an open hash table, it also has a pointer to the next bucket at a position in the hash table
{
	bucket(string sequencePart, int offset)
	{
		m_offset = offset;
		m_sequence = sequencePart;
		m_next = nullptr;
	}
	int m_offset;
	string m_sequence;
	bucket* m_next;
};

class HashTable // class for the actual hash table with necessary member functions 
{
public:
	HashTable();
	~HashTable();
	void insert(int offsetVal, string sequence);
	string returnString(int& offsetVal, string sequence);
private:
	int hashFunc(const string& hashThis) const
	{
		hash<string> str_hash; // the hash function uses the string Hash function within the STL
		unsigned int hashValue = str_hash(hashThis);
		unsigned int bucket = hashValue % HASH_SIZE;
		return bucket;
	}
	bucket* m_buckets[HASH_SIZE];
};

HashTable::HashTable() // iniitialize every bucket in the hash table to nullptr
{
	for (int i = 0; i < HASH_SIZE; i++)
		m_buckets[i] = nullptr;
}

HashTable::~HashTable() // destruct every position in the hash table by deleting all the nodes at each position
{
	for (int i = 0; i < HASH_SIZE; i++)
	{
		bucket*  current = m_buckets[i];
		while (current != nullptr) {
			bucket* previous = current;
			current = current->m_next;
			delete previous;
		}
	}
}

void HashTable::insert(int offsetVal, string sequence)  // insert a sequence into the hash table
{
	int hashPosition = hashFunc(sequence); // uses the string hash function within the STL to determine the position in the hash table the sequence should be inserted at
	if (m_buckets[hashPosition] == nullptr) {
		m_buckets[hashPosition] = new bucket(sequence, offsetVal); // if there are no previous buckets at that position, dynamically allocate the new bucket at that position
	}
	else { // if there is already a bucket at that table, follow the m_next pointers within each bucket to reach the last bucket node at that position and insert the new bucket there
		bucket* newItem = new bucket(sequence, offsetVal);
		bucket* findLast = m_buckets[hashPosition];
		while (findLast->m_next != nullptr)
			findLast = findLast->m_next;
		findLast->m_next = newItem;
	}
}

string HashTable::returnString(int& offsetVal, string sequence) // function to return the string in the hash table that most closely resembles the sequence in the new file
{
	int hashPosition = hashFunc(sequence); // determine what position the new sequence would be if it were in the hash table
	if (m_buckets[hashPosition] == nullptr) // return empty string if the position that the string should be has nothing
		return "";
	else { 
		offsetVal = m_buckets[hashPosition]->m_offset; // if the sequence in the new file is there, return the corresponding sequence in the hash table of the old file and set offsetVal to be the position that the sequence starts at in the old file
		if (m_buckets[hashPosition]->m_sequence == sequence)
			return m_buckets[hashPosition]->m_sequence;
		if (m_buckets[hashPosition]->m_sequence != sequence && m_buckets[hashPosition]->m_next != nullptr) { // if not, follow the m_next pointers within that position in the hash table to see if it is there
			bucket* findLast = m_buckets[hashPosition];
			while (findLast->m_next != nullptr) {
				if (findLast->m_sequence == sequence) {
					offsetVal = findLast->m_offset;
					return findLast->m_sequence;
				}
				findLast = findLast->m_next;
			}
		}
		return ""; // return empty string if the sequence was not any of the buckets at that position in the hash table
	}
}

bool getInt(istream& inf, int& n) // function given in spec
{
	char ch;
	if (!inf.get(ch) || !isascii(ch) || !isdigit(ch))
		return false;
	inf.unget();
	inf >> n;
	return true;
}

bool getCommand(istream& inf, char& cmd, int& length, int& offset) // function given in spec
{
	if (!inf.get(cmd) || (cmd == '\n'  &&  !inf.get(cmd)))
	{
		cmd = 'x';  // signals end of file
		return true;
	}
	char ch;
	switch (cmd)
	{
	case 'A':
		return getInt(inf, length) && inf.get(ch) && ch == ':';
	case 'C':
		return getInt(inf, length) && inf.get(ch) && ch == ',' && getInt(inf, offset);
	}
	return false;
}

void shortenDelta(string delta, ostream& deltaf) // function to shorten the delta file string so that the add commands add more than one character at a time
{
	int addNum = 0;
	string addString;
	for (int i = 0; i < delta.size(); i++) {
		if (delta[i] == '+') { // if the delta string has a + character, which is what the createDelta file includes in place of an A, increment addNum and add the character after into addString
			addNum++;
			addString += delta[i + 1];
			for (int j = i; j < delta.size(); j = j + 2) { // continue until there are no more consecutive + characters, and then add that sequence of adds into the delta file
				if (delta[j + 2] == '+') {
					addNum++;
					addString += delta[j + 3];
				}
				else {
					deltaf << "A" << addNum << ":" << addString;
					addString = "";
					addNum = 0;
					i = j + 1;
					break;
				}
			}
		}
		else { // if the character is not an add character, it is a character that is part of a copy command so just add that into the delta file
			deltaf << delta[i];
		}
	}
}

void createDelta(istream& oldf, istream& newf, ostream& deltaf)
{
	HashTable h; // initialize hash table
	string segment = "";
	string totalOld = "";
	while (getline(oldf, segment)) { // copy all of the characters within oldf into a string called totalOld
		totalOld += segment;
		totalOld += '\n';
	}
	string totalNew = ""; // copy all characters within newF into string called totalNew
	while (getline(newf, segment)) {
		totalNew += segment;
		totalNew += '\n';
	}
	string toAdd = "";
	for (unsigned int i = 0; i < totalOld.size(); i++) { // go along the totalOld and add every sequence of number b_seq characters into the hash table
		toAdd += totalOld[i];
		if (toAdd.size() == b_seq || i == totalOld.size() - 1) {
			h.insert(i - (toAdd.size() - 1), toAdd);
			toAdd = "";
		}
	}
	string toCheck;
	string oldFileSeg;
	int oldOffset = 0;
	string deltaString;
	for (unsigned int j = 0; j < totalNew.size(); j++) // go through the totalNew string
	{
		toCheck += totalNew[j];
		if (toCheck.size() == b_seq || j == totalNew.size() - 1) { // compare every sequence of number b_seq characters to the hash table to see if it is there
			oldFileSeg = h.returnString(oldOffset, toCheck);
			if (oldFileSeg != "" && oldFileSeg == toCheck) {
				int moreCopy = 0;
				int k;
				for (k = 0; k < totalNew.size() - j && k < totalOld.size() - oldOffset; k++) {
					if (totalOld[oldOffset + b_seq + k] == totalNew[j + k + 1]) // if the sequence is there, create a copy command and and add it into the deltaString string
						moreCopy++;
					else {
						break;
					}
				}
				j = j + k;
				deltaString = deltaString + "C" + (to_string(b_seq + moreCopy)) + "," + to_string(oldOffset);
				toCheck = "";
			}
			else {
				deltaString = deltaString + "+" + toCheck[0]; // if the sequence is not there, add the first character by adding the character '+' and the character into the delta string 
				j = j - (toCheck.size() - 1);
				toCheck = "";
			}
		}
	}
	shortenDelta(deltaString, deltaf); // call shortenDelta function to compress all of the consecutive single add commands
}


bool applyDelta(istream& oldf, istream& deltaf, ostream& newf)
{
	char command = 'a';
	int cLength = -1;
	int offset = 2;
	string segment = "";
	string totalOld = "";
	while (getline(oldf, segment)) { // copy all of the characters in the old file into a string
		totalOld += segment;
		totalOld += '\n';
	}
	while (getCommand(deltaf, command, cLength, offset)) { // call getCommand function
		if (command == 'x') // if the command is x, that means the delta file is over so break out of the loop
			break;
		else if (command == 'A') { // if the command is Add, add those strings into the new file
			char c;
			string toAdd = "";
			for (int i = 0; i < cLength; i++) {
				if (deltaf.get(c))
					toAdd += c;
				else
					return false;
			}
			//cerr << toAdd;
			newf << toAdd;
		}
		else if (command == 'C') { // if the command is copy, copy all those strings from the old file into the new one
			if (offset + cLength > totalOld.size()) // check that the copy command is valid with the old file
				return false;
			string toAdd = "";
			for (int i = offset; i < offset + cLength; i++) {
				toAdd += totalOld[i];
			}
			//cerr << toAdd;
			newf << toAdd;
		}
		else if (command != 'A' && command != 'C') // return false if the command is not A or C
			return false;
	}
	return true;
}


int main()
{
	/*ifstream infile("warandpeace1.txt");
	ofstream outfile("delta.txt");
	ifstream old("warandpeace2.txt");
	createDelta(infile, old, outfile);*/

	ifstream infile("warandpeace1.txt");
	ifstream delta("delta.txt");
	ofstream newFile("3.txt");
	if (applyDelta(infile, delta, newFile) == true) {
		cout << "good";
	}
}