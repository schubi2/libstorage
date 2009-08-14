
#include <fstream>
#include <algorithm>

#include "storage/AppUtil.h"
#include "storage/StorageTypes.h"
#include "storage/AsciiFile.h"
#include "storage/Regex.h"
#include "storage/StorageTmpl.h"
#include "storage/Volume.h"
#include "storage/EtcFstab.h"


namespace storage
{
    using namespace std;


EtcFstab::EtcFstab(const string& pfx, bool rootMounted) 
    : prefix(pfx)
{
    y2mil("prefix:" << pfx << " rootMounted:" << rootMounted);
    if (rootMounted)
	readFiles();
}

void
EtcFstab::readFiles()
    {
    list<Entry>::iterator i = co.begin();
    while( i!=co.end() )
	{
	if( i->op!=Entry::ADD )
	    i=co.erase(i);
	else
	    ++i;
	}
    y2mil("entries:" << co.size());

    string file = prefix+"/fstab";
    ifstream mounts( file.c_str() );
    classic(mounts);
    string line;
    getline( mounts, line );
    while( mounts.good() )
	{
	y2mil( "line:\"" << line << "\"" );
	list<string> l = splitString( line );
	list<string>::const_iterator i = l.begin();
	if( l.begin()!=l.end() && i->find( '#' )!=0 )
	    {
	    Entry *p = new Entry;
	    if( i!=l.end() )
		p->old.device = p->old.dentry = *i++;
	    if( i!=l.end() )
		p->old.mount = *i++;
	    if( i!=l.end() )
		{
		p->old.fs = *i++;
		if( p->old.fs=="crypt" )
		    {
		    p->old.dmcrypt = true;
		    p->old.encr = ENC_LUKS;
		    }
		else if( p->old.fs=="ntfs-3g" )
		    p->old.fs=="ntfs";
		}
	    if( i!=l.end() )
		p->old.opts = splitString( *i++, "," );
	    if( i!=l.end() )
		*i++ >> p->old.freq;
	    if( i!=l.end() )
		*i++ >> p->old.passno;
	    p->old.calcDependent();
	    p->nnew = p->old;
	    co.push_back( *p );
	    delete p;
	    }
	getline( mounts, line );
	}
    mounts.close();

    file = prefix+"/cryptotab";
    mounts.clear();
    mounts.open( file.c_str() );
    getline( mounts, line );
    while( mounts.good() )
	{
	y2mil( "line:\"" << line << "\"" );
	list<string> l = splitString( line );
	list<string>::const_iterator i = l.begin();
	Entry *p = new Entry;

	p->old.loop = p->old.cryptotab = true;
	if( i!=l.end() )
	    p->old.loop_dev = *i++;
	if( i!=l.end() )
	    p->old.device = p->old.dentry = *i++;
	if( i!=l.end() )
	    p->old.mount = *i++;
	if( i!=l.end() )
	    {
	    p->old.fs = *i++;
	    if( p->old.fs=="ntfs-3g" )
		p->old.fs=="ntfs";
	    }
	if( i!=l.end() )
	    p->old.encr = Volume::toEncType( *i++ );
	if( i!=l.end() )
	    p->old.opts = splitString( *i++, "," );
	p->nnew = p->old;
	co.push_back( *p );
	delete p;

	getline( mounts, line );
	}
    mounts.close();

    file = prefix+"/crypttab";
    mounts.clear();
    mounts.open( file.c_str() );
    getline( mounts, line );
    while( mounts.good() )
	{
	y2mil( "line:\"" << line << "\"" );
	list<string> l = splitString( line );
	if( l.size()>=3 )
	    {
	    list<string>::const_iterator i = l.begin();
	    list<Entry>::iterator e = co.begin();
	    string dmdev = "/dev/mapper/" + *i;
	    ++i;
	    y2mil( "dmdev:" << dmdev );
	    while( e!=co.end() && e->old.device!=dmdev )
		++e;
	    Entry *p = NULL;
	    if( e==co.end() )
		{
		co.push_back( Entry() );
		p = &(co.back());
		p->old.dentry = *i;
		}
	    else
		p = &(*e);
	    p->old.dmcrypt = p->old.crypttab = true;
	    p->old.noauto = false;
	    p->old.encr = ENC_LUKS;
	    p->old.device = *i;
	    list<string>::iterator li =  p->old.opts.begin();
	    while( li != p->old.opts.end() )
		{
		if( *li == "noauto" )
		    li = p->old.opts.erase(li);
		else
		    ++li;
		}
	    ++i;
	    if( *i != "none" )
		p->old.cr_key = *i;
	    ++i;
	    if( i!=l.end() )
		{
		if( *i != "none" )
		    p->old.cr_opts = *i;
		++i;
		}
	    p->nnew = p->old;
	    y2mil( "after crtab " << p->nnew );
	    }
	getline( mounts, line );
	}
    mounts.close();

    y2mil("entries:" << co.size());
    }


void
FstabEntry::calcDependent()
    {
    list<string>::const_iterator beg = opts.begin();
    list<string>::const_iterator end = opts.end();

    noauto = find( beg, end, "noauto" ) != end;

    list<string>::const_iterator i = find_if( beg, end, string_starts_with("loop") );
    if( i!=end )
	{
	loop = true;
	string::size_type pos = i->find("=");
	if( pos!=string::npos )
	    {
	    loop_dev = i->substr( pos+1 );
	    }
	}
    i = find_if( beg, end, string_starts_with("encryption=") );
    if( i!=end )
	{
	string::size_type pos = i->find("=");
	if( pos!=string::npos )
	    {
	    encr = Volume::toEncType( i->substr( pos+1 ) );
	    }
	}

	if (boost::starts_with(device, "LABEL="))
	{
	    mount_by = MOUNTBY_LABEL;
	    device.erase();
	}
	else if (boost::starts_with(device, "UUID="))
	{
	    mount_by = MOUNTBY_UUID;
	    device.erase();
	}
	else if (boost::starts_with(device, "/dev/disk/by-id/"))
        {
	    mount_by = MOUNTBY_ID;
	}
	else if (boost::starts_with(device, "/dev/disk/by-path/"))
        {
	    mount_by = MOUNTBY_PATH;
	}
	else if (boost::starts_with(device, "/dev/disk/by-label/"))
	{
	    mount_by = MOUNTBY_LABEL;
	}
	else if (boost::starts_with(device, "/dev/disk/by-uuid/"))
	{
	    mount_by = MOUNTBY_UUID;
	}

    dmcrypt = encr==ENC_LUKS;
    cryptotab = !noauto && encr!=ENC_NONE && !dmcrypt;
    crypttab = !noauto && dmcrypt;
    }


bool
EtcFstab::findDevice( const string& dev, FstabEntry& entry ) const
    {
    list<Entry>::const_iterator i = co.begin();
    while( i!=co.end() && i->nnew.device != dev )
	++i;
    if( i!=co.end() )
	entry = i->nnew;
    return( i!=co.end() );
    }

bool
EtcFstab::findDevice( const list<string>& dl, FstabEntry& entry ) const
    {
    list<Entry>::const_iterator i = co.begin();
    while( i!=co.end() &&
           find( dl.begin(), dl.end(), i->nnew.device )==dl.end() )
	++i;
    if( i!=co.end() )
	entry = i->nnew;
    return( i!=co.end() );
    }

bool
EtcFstab::findMount( const string& mount, FstabEntry& entry ) const
    {
    list<Entry>::const_iterator i = co.begin();
    while( i!=co.end() && i->nnew.mount != mount )
	++i;
    if( i!=co.end() )
	entry = i->nnew;
    return( i!=co.end() );
    }

int EtcFstab::changeRootPrefix( const string& prfix )
    {
    y2mil("new prefix:" << prfix);
    int ret = 0;
    if( prfix != prefix )
	{
	list<Entry>::const_iterator i = co.begin();
	while( i!=co.end() && (i->op==Entry::ADD || i->op==Entry::NONE) )
	    ++i;
	if( i==co.end() )
	    {
	    prefix = prfix;
	    readFiles();
	    }
	else
	    ret = FSTAB_CHANGE_PREFIX_IMPOSSIBLE;
	}
    y2mil("ret:" << ret);
    return( ret );
    }

void EtcFstab::setDevice( const FstabEntry& entry, const string& device )
    {
    list<Entry>::iterator i = co.begin();
    while( i!=co.end() && i->old.dentry != entry.dentry )
	++i;
    if( i!=co.end() )
	i->nnew.device = i->old.device = device;
    }


    bool
    EtcFstab::findUuidLabel(const string& uuid, const string& label,
			    FstabEntry& entry) const
    {
	y2mil("uuid:" << uuid << " label:" << label);
	list<Entry>::const_iterator i = co.end();
	if (!uuid.empty())
	{
	    string dentry = "UUID=" + uuid;
	    i = co.begin();
	    while (i != co.end() && i->nnew.dentry != dentry)
		++i;
	}
	if (i == co.end() && !uuid.empty())
	{
	    string dentry = "/dev/disk/by-uuid/" + uuid;
	    i = co.begin();
	    while (i != co.end() && i->nnew.dentry != dentry)
		++i;
	}
	if (i == co.end() && !label.empty())
	{
	    string dentry = "LABEL=" + label;
	    i = co.begin();
	    while (i != co.end() && i->nnew.dentry != dentry)
		++i;
	}
	if (i == co.end() && !label.empty())
	{
	    string dentry = "/dev/disk/by-label/" + udevEncode(label);
	    i = co.begin();
	    while (i != co.end() && i->nnew.dentry != dentry)
		++i;
	}
	if (i != co.end())
	    entry = i->nnew;
	y2mil("ret:" << (i != co.end()));
	return i != co.end();
    }


bool
EtcFstab::findIdPath( const std::list<string>& id, const string& path,
		      FstabEntry& entry ) const
    {
    y2mil( "id:" << id << " path:" << path );
    list<Entry>::const_iterator i = co.begin();
    if( !id.empty() )
	{
	while( i!=co.end() && 
	       find( id.begin(), id.end(), i->nnew.dentry )==id.end() )
	    ++i;
	}
    else 
	i = co.end();
    if( i==co.end() && !path.empty() )
	{
	i = co.begin();
	while( i!=co.end() && i->nnew.dentry != path )
	    ++i;
	}
    if( i!=co.end())
	entry = i->nnew;
    y2mil("ret:" << (i != co.end()));
    return( i!=co.end() );
    }

int EtcFstab::removeEntry( const FstabEntry& entry )
    {
	y2mil("dentry:" << entry.dentry << " mount:" << entry.mount);
    list<Entry>::iterator i = co.begin();
    while( i != co.end() &&
           (i->op==Entry::REMOVE || i->nnew.device != entry.device) )
	++i;
    if( i != co.end() )
	{
	if( i->op != Entry::ADD )
	    i->op = Entry::REMOVE;
	else
	    co.erase( i );
	}
    return( (i != co.end())?0:FSTAB_ENTRY_NOT_FOUND );
    }

int EtcFstab::updateEntry( const FstabChange& entry )
    {
	y2mil("dentry:" << entry.dentry << " mount:" << entry.mount);
    list<Entry>::iterator i = co.begin();
    bool found = false;
    while( i != co.end() && !found )
	{
	if( i->op==Entry::REMOVE ||
	    (i->op!=Entry::ADD && entry.device!=i->old.device) ||
	    (i->op==Entry::ADD && entry.device!=i->nnew.device) )
	    ++i;
	else
	    found = true;
	}
    if( i != co.end() )
	{
	if( i->op==Entry::NONE )
	    i->op = Entry::UPDATE;
	i->nnew = entry;
	}
    int ret = (i != co.end())?0:FSTAB_ENTRY_NOT_FOUND;
    y2mil("ret:" << ret);
    return( ret );
    }

int EtcFstab::addEntry( const FstabChange& entry )
    {
	y2mil("dentry:" << entry.dentry << " mount:" << entry.mount);
    Entry e;
    e.op = Entry::ADD;
    e.nnew = entry;
    y2mil( "e.nnew " << e.nnew );
    co.push_back( e );
    return( 0 );
    }

AsciiFile* EtcFstab::findFile( const FstabEntry& e, AsciiFile*& fstab,
                               AsciiFile*& cryptotab, int& lineno ) const
    {
	y2mil("dentry:" << e.dentry << " mount:" << e.mount << " fstab:" << fstab <<
	      " cryptotab:" << cryptotab);
    AsciiFile* ret=NULL;
    string reg;
    if( e.cryptotab )
	{
	if( cryptotab==NULL )
	    cryptotab = new AsciiFile( prefix + "/cryptotab", true );
	ret = cryptotab;
	reg = "[ \t]" + e.dentry + "[ \t]";
	}
    else
	{
	if( fstab==NULL )
	    fstab = new AsciiFile( prefix + "/fstab" );
	ret = fstab;
	reg = "^[ \t]*" + e.dentry + "[ \t]";
	}
    lineno = ret->find_if_idx(regex_matches(reg));
    y2mil("fstab:" << fstab << " cryptotab:" << cryptotab << " lineno:" << lineno);
    return( ret );
    }

int EtcFstab::findPrefix( const AsciiFile& tab, const string& mount ) const
    {
    bool crypto = tab.name().find( "/cryptotab" )>=0;
    y2mil("file:" << tab.name() << " mount:" << mount << " crypto:" << crypto);
    string reg = "^[ \t]*[^ \t]+";
    if( crypto )
	reg += "[ \t]+[^ \t]+";
    reg += "[ \t]+" + mount;
    if( mount.length()>0 && mount[mount.length()-1] != '/' )
	reg += "/";
    int lineno = tab.find_if_idx(regex_matches(reg));
    y2mil("reg:" << reg << " lineno:" << lineno);
    return( lineno );
    }

bool EtcFstab::findCrtab( const FstabEntry& e, const AsciiFile& tab, 
                          int& lineno ) const
    {
    y2mil("dev:" << e.device);
    string reg = "^[ \t]*[^ \t]+[ \t]+" + e.device + "[ \t]";
    lineno = tab.find_if_idx(regex_matches(reg));
    if( lineno<0 )
	{
	reg = "^[ \t]*" + e.dentry + "[ \t]";
	lineno = tab.find_if_idx(regex_matches(reg));
	}
    y2mil("reg:" << reg << " lineno:" << lineno);
    return( lineno>=0 );
    }

bool EtcFstab::findCrtab( const string& dev, const AsciiFile& tab, 
                          int& lineno ) const
    {
    y2mil("dev:" << dev.c_str());
    string reg = "^[ \t]*[^ \t]+[ \t]+" + dev + "[ \t]";
    lineno = tab.find_if_idx(regex_matches(reg));
    y2mil("reg:" << reg << " lineno:" << lineno);
    return( lineno>=0 );
    }

    list<string> EtcFstab::makeStringList(const FstabEntry& e) const
    {
	list<string> ls;
    if( e.cryptotab )
	{
	ls.push_back( e.loop_dev );
	}
    ls.push_back( e.dentry );
    ls.push_back( e.mount );
    if( e.dmcrypt && e.noauto )
	ls.push_back( "crypt" );
    else
	{
	ls.push_back( (e.fs!="ntfs")?e.fs:"ntfs-3g" );
	}
    if( e.cryptotab )
	{
	ls.push_back( Volume::encTypeString(e.encr) );
	}
    ls.push_back( boost::join( e.opts, "," ) );
    if( (e.dmcrypt&&e.mount!="swap") &&
        find( e.opts.begin(), e.opts.end(), "noauto" )==e.opts.end() )
	{
	if( ls.back() == "defaults" )
	    ls.back() = "noauto";
	else
	    ls.back() += ",noauto";
	}
    if( !e.cryptotab )
	{
	ls.push_back( decString(e.freq) );
	ls.push_back( decString(e.passno) );
	}
	return ls;
    }

string EtcFstab::createLine( const list<string>& ls, unsigned fields, 
                             unsigned* flen ) const
    {
    string ret;
    unsigned count=0;
    for( list<string>::const_iterator i=ls.begin(); i!=ls.end(); ++i )
	{
	if( i != ls.begin() )
	    ret += " ";
	ret += *i;
	if( count<fields && i->size()<flen[count] )
	    {
	    ret.replace( ret.size(), 0, flen[count]-i->size(), ' ' );
	    }
	count++;
	}
    y2mil( "ret:" << ret );
    return( ret );
    }

string EtcFstab::createTabLine( const FstabEntry& e ) const
    {
	y2mil("dentry:" << e.dentry << " mount:" << e.mount << "device:" << e.device);
    const list<string> ls = makeStringList(e);
    unsigned max_fields = e.cryptotab ? lengthof(cryptotabFields)
			      : lengthof(fstabFields);
    unsigned* fields = e.cryptotab ? cryptotabFields : fstabFields;
    return createLine(ls, max_fields, fields);
    }

    list<string> EtcFstab::makeCrStringList(const FstabEntry& e) const
    {
	list<string> ls;
    ls.push_back( e.dentry.substr(e.dentry.rfind( '/' )+1) );
    string tmp = e.device;
    ls.push_back( tmp );
    tmp = e.cr_key;
    if( e.tmpcrypt )
	tmp = "/dev/urandom";
    ls.push_back( tmp.empty()?"none":tmp );
    tmp = e.cr_opts;
    list<string>::iterator i;
    list<string> tls = splitString( tmp );
    if( e.mount=="swap" && 
	find( tls.begin(), tls.end(), "swap" )==tls.end() )
	tls.push_back("swap");
    else if( e.mount!="swap" && 
	     (i=find( tls.begin(), tls.end(), "swap" ))!=tls.end() )
	tls.erase(i);
    bool need_tmp = e.tmpcrypt && e.mount!="swap";
    if( need_tmp && find( tls.begin(), tls.end(), "tmp" )==tls.end() )
	tls.push_back("tmp");
    else if( !need_tmp && (i=find( tls.begin(), tls.end(), "tmp" ))!=tls.end() )
	tls.erase(i);
    tmp = boost::join( tls, "," );
    ls.push_back( tmp.empty()?"none":tmp );
	return ls;
    }

string EtcFstab::createCrtabLine( const FstabEntry& e ) const
    {
	y2mil("dentry:" << e.dentry << " mount:" << e.mount << " device:" << e.device);
    const list<string> ls = makeCrStringList(e);
    return createLine(ls, lengthof(crypttabFields), crypttabFields);
    }


    void
    EtcFstab::getFileBasedLoops(const string& prefix, list<FstabEntry>& l) const
    {
	l.clear();
	for (list<Entry>::const_iterator i = co.begin(); i != co.end(); ++i)
	{
	    if (i->op == Entry::NONE)
	    {
		string lfile = prefix + i->old.device;
		if (checkNormalFile(lfile))
		    l.push_back(i->old);
	    }
	}
    }


    list<FstabEntry>
    EtcFstab::getEntries() const
    {
	list<FstabEntry> ret;
	for (list<Entry>::const_iterator i = co.begin(); i != co.end(); ++i)
	{
	    if (i->op == Entry::NONE)
		ret.push_back(i->old);
	}
	return ret;
    }


string EtcFstab::updateLine( const list<string>& ol,
                             const list<string>& nl, const string& oldline ) const
    {
    string line( oldline );
    list<string>::const_iterator oi = ol.begin();
    list<string>::const_iterator ni = nl.begin();
    string::size_type pos = line.find_first_not_of( app_ws );
    string::size_type posn = line.find_first_of( app_ws, pos );
    posn = line.find_first_not_of( app_ws, posn );
    while( ni != nl.end() )
	{
	if( *ni != *oi || oi==ol.end() )
	    {
	    string nstr = *ni;
	    if( posn != string::npos )
		{
		unsigned diff = posn-pos-1;
		if( diff > nstr.size() )
		    {
		    nstr.replace( nstr.size(), 0, diff-nstr.size(), ' ' );
		    }
		line.replace( pos, posn-1-pos, nstr );
		if( nstr.size()>diff )
		    posn += nstr.size()-diff;
		}
	    else if( pos!=string::npos )
		{
		line.replace( pos, posn-pos, nstr );
		}
	    else 
		{
		line += ' ';
		line += nstr;
		}
	    }
	pos = posn;
	posn = line.find_first_of( app_ws, pos );
	posn = line.find_first_not_of( app_ws, posn );
	if( oi!=ol.end() )
	    ++oi;
	++ni;
	}
    return( line );
    }


int EtcFstab::flush()
    {
    int ret = 0;
    AsciiFile *fstab = NULL;
    AsciiFile *cryptotab = NULL;
    AsciiFile *cur = NULL;
    AsciiFile crypttab( prefix + "/crypttab", true );
    if (!co.empty() && !checkDir(prefix))
	createPath( prefix );

    list<Entry>::iterator i = co.begin();
    while( i!=co.end() && ret==0 )
	{
	switch( i->op )
	    {
	    case Entry::REMOVE:
	    {
		int lineno;
		cur = findFile( i->old, fstab, cryptotab, lineno );
		if( lineno>=0 )
		{
		    cur->remove( lineno, 1 );
		    if( cur==fstab && i->old.crypttab && 
		        findCrtab( i->old, crypttab, lineno ))
			crypttab.remove( lineno, 1 );
		}
		else if (i->old.crypttab && findCrtab(i->old, crypttab, lineno))
		{
		    crypttab.remove( lineno, 1 );
		}
		else
		{
		    ret = FSTAB_REMOVE_ENTRY_NOT_FOUND;
		}
		i = co.erase( i );
	    } break;

	    case Entry::UPDATE:
	    {
		int lineno;
		cur = findFile( i->old, fstab, cryptotab, lineno );
		if( lineno<0 )
		    cur = findFile( i->nnew, fstab, cryptotab, lineno );
		if( lineno>=0 )
		    {
		    string line;
		    if( i->old.cryptotab != i->nnew.cryptotab )
			{
			cur->remove( lineno, 1 );
			cur = findFile( i->nnew, fstab, cryptotab, lineno );
			line = createTabLine( i->nnew );
			cur->append( line );
			}
		    else
			{
			line = (*cur)[lineno];
			const list<string> nl = makeStringList(i->nnew);
			const list<string> ol = makeStringList(i->old);
			line = updateLine( ol, nl, line );
			(*cur)[lineno] = line;
			}
		    if( i->old.crypttab > i->nnew.crypttab && 
		        findCrtab( i->old, crypttab, lineno ))
			crypttab.remove( lineno, 1 );
		    if( i->nnew.crypttab )
			{
			line = createCrtabLine( i->nnew );
			if( findCrtab( i->old, crypttab, lineno ) ||
			    findCrtab( i->nnew, crypttab, lineno ))
			    {
			    line = crypttab[lineno];
			    const list<string> nl = makeCrStringList(i->nnew);
			    const list<string> ol = makeCrStringList(i->old);
			    line = updateLine( ol, nl, line );
			    crypttab[lineno] = line;
			    }
			else
			    crypttab.append( line );
			}
		    i->old = i->nnew;
		    i->op = Entry::NONE;
		    }
		else
		    ret = FSTAB_UPDATE_ENTRY_NOT_FOUND;
	    } break;

	    case Entry::ADD:
	    {
		int lineno;
		cur = findFile( i->nnew, fstab, cryptotab, lineno );
		string line = createTabLine( i->nnew );
		string before_dev;
		if( lineno<0 )
		    {
		    lineno = findPrefix( *cur, i->nnew.mount );
		    if( lineno>=0 )
			{
			before_dev = extractNthWord( 0, (*cur)[lineno] );
			if (!i->nnew.mount.empty())
			    cur->insert( lineno, line );
			}
		    else
		    {
			if (!i->nnew.mount.empty())
			    cur->append( line );
		    }
		    }
		else
		    {
		    y2war( "replacing line:" << (*cur)[lineno] );
		    (*cur)[lineno] = line;
		    }
		if( i->nnew.crypttab )
		    {
		    line = createCrtabLine( i->nnew );
		    if( findCrtab( i->nnew, crypttab, lineno ))
			{
			crypttab[lineno] = line;
			}
		    else if( !before_dev.empty() &&
		             findCrtab( before_dev, crypttab, lineno ))
			{
			crypttab.insert( lineno, line );
			}
		    else
			{
			crypttab.append( line );
			}
		    }
		i->old = i->nnew;
		i->op = Entry::NONE;
	    } break;

	    default:
		break;
	    }
	++i;
	}
    if( fstab != NULL )
	{
	fstab->save();
	delete( fstab );
	}
    if( cryptotab != NULL )
	{
	cryptotab->save();
	delete( cryptotab );
	}
    if( true )
        {
	crypttab.save();
	}
    AsciiFile(prefix + "/fstab").logContent();
    AsciiFile(prefix + "/cryptotab").logContent();
    AsciiFile(prefix + "/crypttab").logContent();
    y2mil("ret:" << ret);
    return( ret );
    }

string EtcFstab::addText( bool doing, bool crypto, const string& mp ) const
    {
    const char* file = crypto?"/etc/cryptotab":"/etc/fstab";
    string txt;
    if( doing )
	{
	// displayed text during action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Adding entry for mount point %1$s to %2$s"),
		       mp.c_str(), file );
	}
    else
	{
	// displayed text before action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Add entry for mount point %1$s to %2$s"),
		       mp.c_str(), file );
	}
    return( txt );
    }

string EtcFstab::updateText( bool doing, bool crypto, const string& mp ) const
    {
    const char* file = crypto?"/etc/cryptotab":"/etc/fstab";
    string txt;
    if( doing )
	{
	// displayed text during action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Updating entry for mount point %1$s in %2$s"),
		       mp.c_str(), file );
	}
    else
	{
	// displayed text before action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Update entry for mount point %1$s in %2$s"),
		       mp.c_str(), file );
	}
    return( txt );
    }

string EtcFstab::removeText( bool doing, bool crypto, const string& mp ) const
    {
    const char* file = crypto?"/etc/cryptotab":"/etc/fstab";
    string txt;
    if( doing )
	{
	// displayed text during action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Removing entry for mount point %1$s from %2$s"),
		       mp.c_str(), file );
	}
    else
	{
	// displayed text before action, %1$s is replaced by mount point e.g. /home
	// %2$s is replaced by a pathname e.g. /etc/fstab
	txt = sformat( _("Remove entry for mount point %1$s from %2$s"),
		       mp.c_str(), file );
	}
    return( txt );
    }


    std::ostream& operator<<(std::ostream& s, const FstabEntry& v)
    {
	s << "device:" << v.device
	  << " dentry:" << v.dentry << " mount:" << v.mount
	  << " fs:" << v.fs << " opts:" << boost::join(v.opts, ",")
	  << " freq:" << v.freq << " passno:" << v.passno;
	if( v.noauto )
	    s << " noauto";
	if( v.cryptotab )
	    s << " cryptotab";
	if( v.crypttab )
	    s << " crypttab";
	if( v.tmpcrypt )
	    s << " tmpcrypt";
	if( v.loop )
	    s << " loop";
	if( v.dmcrypt )
	    s << " dmcrypt";
	if( !v.loop_dev.empty() )
	    s << " loop_dev:" << v.loop_dev;
	if( !v.cr_key.empty() )
	    s << " cr_key:" << v.cr_key;
	if( !v.cr_opts.empty() )
	    s << " cr_opts:" << v.cr_opts;
	if( v.encr != storage::ENC_NONE )
	    s << " encr:" << v.encr;
	return s;
    }


    std::ostream& operator<<(std::ostream& s, const FstabChange& v)
    {
	s << "device:" << v.device
	  << " dentry:" << v.dentry << " mount:" << v.mount
	  << " fs:" << v.fs << " opts:" << boost::join(v.opts, ",")
	  << " freq:" << v.freq << " passno:" << v.passno;
	if( !v.loop_dev.empty() )
	    s << " loop_dev:" << v.loop_dev;
	if( v.encr != storage::ENC_NONE )
	    s << " encr:" << v.encr;
	if( v.tmpcrypt )
	    s << " tmpcrypt";
	return s;
    }


unsigned EtcFstab::fstabFields[] = { 20, 20, 10, 21, 1, 1 };
unsigned EtcFstab::cryptotabFields[] = { 11, 15, 20, 10, 10, 1 };
unsigned EtcFstab::crypttabFields[] = { 15, 20, 10, 1 };

}