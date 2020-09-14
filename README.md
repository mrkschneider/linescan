# linescan - fast character and newline search in buffers

## Summary
linescan allows to find locations of a specific character in a buffer until a newline is encountered. For example, this can be useful to find the locations of a delimiter character in a line read from a CSV file. Function implementations are in large parts derived from the GNU C library; therefore, this library is provided under the same license (GNU Lesser General Public License 2.1).

## Dependencies
To compile linescan, create a file called 'Makefile.env' in the project directory, setting environment variables to the paths of the following libraries:

* **CXXTESTDIR** - [CxxTest](http://cxxtest.com/) (4.4+)

## Disclaimer
This is a project I maintain for fun in my free time, with no implied guarantees regarding support, completeness or fitness for any particular purpose. I am grateful for suggestions, bug reports or pull requests, but I might not respond to every request. 


