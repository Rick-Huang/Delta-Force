# Delta-Force

A tool that can take two text files and create the smallest possible “delta” file that recorded all of the

differences and similarities between the two files.

Parses original text into sections into an open hash table which were then compared section by section with the new text

file. Copies and appended characters would be outputted to the “delta” file.
