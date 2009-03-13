#ifndef ASCII_FILE_H
#define ASCII_FILE_H


#include <string>
#include <vector>
#include <algorithm>


namespace storage
{
    using std::string;
    using std::vector;


    class AsciiFile
    {
    public:

	AsciiFile(const char* name, bool remove_empty = false);
	AsciiFile(const string& name, bool remove_empty = false);
	~AsciiFile();

	string name() const { return Name_C; }

	bool load();
	bool save();

	bool insertFile(const AsciiFile& File_Cv, unsigned int BeforeLine_iv = 0);
	bool appendFile(const AsciiFile& File_Cv);
	bool insertFile( const string& Name_Cv, unsigned int BeforeLine_iv=0 );
	bool appendFile( const string& Name_Cv );

	void append( const string& Line_Cv );
	void append( const vector<string>& Lines_Cv );
	void insert( unsigned int Before_iv, const string& Line_Cv );
	void clear();
	void remove( unsigned int Start_iv, unsigned int Cnt_iv );
	void replace( unsigned int Start_iv, unsigned int Cnt_iv,
		      const string& Line_Cv );
	void replace( unsigned int Start_iv, unsigned int Cnt_iv,
		      const vector<string>& Line_Cv );

	const string& operator []( unsigned int Index_iv ) const;
	string& operator []( unsigned int Index_iv );

	template <class Pred>
	int find_if(Pred pred) const
	{
	    vector<string>::const_iterator it = std::find_if(Lines_C.begin(), Lines_C.end(), pred);
	    if (it == Lines_C.end())
		return -1;
	    return std::distance(Lines_C.begin(), it);
	}

	unsigned numLines() const { return Lines_C.size(); }

	const vector<string>& lines() const { return Lines_C; }

    protected:

	bool appendFile( const string& Name_Cv, vector<string>& Lines_Cr ) const;
	bool appendFile(const AsciiFile& File_Cv, vector<string>& Lines_Cr) const;
	void removeLastIf(string& Text_Cr, char Char_cv) const;

	const string Name_C;
	const bool remove_empty;

	vector<string> Lines_C;

    };

}


#endif
