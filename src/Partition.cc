/*
  Textdomain    "storage"
*/

#include <sstream>

#include "y2storage/Partition.h"
#include "y2storage/AppUtil.h"
#include "y2storage/Disk.h"

using namespace storage;
using namespace std;

Partition::Partition( const Disk& d, unsigned PNr, unsigned long long SizeK,
                      unsigned long Start, unsigned long CSize,
		      PartitionType Type, unsigned Id, bool Boot )
    : Volume( d, PNr, SizeK ), reg( Start, CSize )
    {
    bootflag = Boot;
    idt = orig_id = Id;
    typ = Type;
    orig_num = num;
    y2milestone( "constructed partition %s on disk %s", dev.c_str(),
                 cont->name().c_str() );
    }

Partition::Partition( const Disk& d, const string& Data ) :
        Volume( d, 0, 0 )
    {
    string ts, rs;
    istringstream i( Data );
    i >> num >> dev >> size_k >> mjr >> mnr >> reg >>
	 hex >> idt >> dec >> ts >> rs;
    orig_size_k = size_k;
    orig_num = num;
    orig_id = idt;
    nm = dev;
    undevDevice(nm);	// strip "/dev/"
    if( ts == "extended" )
	typ = EXTENDED;
    else if( ts == "logical" )
	typ = LOGICAL;
    else
	typ = PRIMARY;
    if( rs == "boot" )
	bootflag = true;
    else
	bootflag = false;
    y2milestone( "constructed partition %s on disk %s", dev.c_str(),
                 cont->name().c_str() );
    }

bool Partition::intersectArea( const Region& r, unsigned fuzz ) const
    {
    return( r.intersect( reg ).len()>fuzz );
    }

bool Partition::contains( const Region& r, unsigned fuzz ) const
    {
    return( (r.len() - reg.intersect( r ).len()) <= fuzz );
    }

void Partition::changeNumber( unsigned new_num )
    {
    if( new_num!=num )
	{
	if( orig_num==num )
	    {
	    orig_num = num;
	    }
	num = new_num;
	if( created() )
	    {
	    orig_num = num;
	    }
	setNameDev();
	getMajorMinor( dev, mjr, mnr );
	}
    }

void Partition::changeId( unsigned new_id )
    {
    if( new_id!=idt )
	{
	if( orig_id==idt )
	    {
	    orig_id = idt;
	    }
	idt = new_id;
	if( created() )
	    {
	    orig_id = idt;
	    }
	}
    }

void Partition::changeIdDone()
    {
    orig_id = idt;
    }

void Partition::unChangeId()
    {
    idt = orig_id;
    }

void Partition::changeRegion( unsigned long Start, unsigned long CSize,
			      unsigned long long SizeK )
    {
    reg = Region( Start, CSize );
    size_k = orig_size_k = SizeK;
    }

bool Partition::canUseDevice() const
    {
    bool ret = Volume::canUseDevice();
    if( ret )
	ret = type()!=EXTENDED;
    return( ret );
    }

void Partition::setResizedSize( unsigned long long SizeK ) 
    {
    Volume::setResizedSize(SizeK);
    reg = Region( cylStart(), disk()->kbToCylinder(SizeK) );
    }

void Partition::forgetResize() 
    { 
    Volume::forgetResize();
    reg = Region( cylStart(), disk()->kbToCylinder(size_k) );
    }


ostream& Partition::logData( ostream& file ) const
    {
    file << num << " " << dev << " " << size_k << " " <<  mjr << " "
         << mnr << " ";
    file << reg.start() << " " << reg.len() << " " << hex << idt << dec;
    if( typ == LOGICAL )
	file << " logical";
    else if( typ == EXTENDED )
	file << " extended";
    else
	file << " primary";
    if( bootflag )
	file << " boot";
    if( orig_num!=num )
	file << " OrigNr:" << orig_num;
    return( file );
    }

string Partition::setTypeText( bool doing ) const
    {
    string txt;
    string d = dev;
    if( doing )
        {
        // displayed text during action, %1$s is replaced by partition name (e.g. /dev/hda1),
        // %2$s is replaced by hexadecimal number (e.g. 8E)
        txt = sformat( _("Setting type of partition %1$s to %2$X"),
                      d.c_str(), id() );
        }
    else
        {
        // displayed text before action, %1$s is replaced by partition name (e.g. /dev/hda1),
        // %2$s is replaced by hexadecimal number (e.g. 8E)
        txt = sformat( _("Set type of partition %1$s to %2$X"),
                      d.c_str(), id() );
        }
    return( txt );
    }

const Disk* const Partition::disk() const
    {
    return(dynamic_cast<const Disk* const>(cont));
    }

int Partition::setFormat( bool val, storage::FsType new_fs )
{
    int ret = 0;
    y2milestone( "device:%s val:%d fs:%s", dev.c_str(), val,
		 fs_names[new_fs].c_str() );
    if( typ==EXTENDED )
	ret = VOLUME_FORMAT_EXTENDED_UNSUPPORTED;
    else
	ret = Volume::setFormat( val, new_fs );
    y2milestone( "ret:%d", ret );
    return( ret );
}

int Partition::changeMount( const string& val )
    {
    int ret = 0;
    y2milestone( "device:%s val:%s", dev.c_str(), val.c_str() );
    if( typ==EXTENDED )
	ret = VOLUME_MOUNT_EXTENDED_UNSUPPORTED;
    else
	ret = Volume::changeMount( val );
    y2milestone( "ret:%d", ret );
    return( ret );
    }

bool Partition::isWindows() const
    {
    return( idt==6 || idt==0xb || idt==ID_DOS || idt==0xe || idt==1 || idt==4 ||
	    idt==ID_NTFS || idt==0x17 );
    }

string Partition::removeText( bool doing ) const
    {
    string txt;
    string d = dev;
    if( orig_num!=num )
	{
	d = disk()->getPartName( orig_num );
	}
    if( doing )
	{
	// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
	txt = sformat( _("Deleting partition %1$s"), d.c_str() );
	}
    else
	{
	d.erase( 0, 5 );
	if( isWindows() )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Delete Windows partition %1$s (%2$s)"), d.c_str(),
	                      sizeString().c_str() );
	    }
	else
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Delete partition %1$s (%2$s)"), d.c_str(),
	                      sizeString().c_str() );
	    }
	}
    return( txt );
    }

string Partition::createText( bool doing ) const
    {
    string txt;
    string d = dev;
    if( doing )
	{
	// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
	txt = sformat( _("Creating partition %1$s"), d.c_str() );
	}
    else
	{
	if( typ==EXTENDED )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Create extended partition %1$s (%2$s)"),
	                   d.c_str(), sizeString().c_str() );
	    }
	else if( mp=="swap" )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Create swap partition %1$s (%2$s)"),
	                   d.c_str(), sizeString().c_str() );
	    }
	else if( mp=="/" )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    // %3$s is replaced by file system type (e.g. reiserfs)
	    txt = sformat( _("Create root partition %1$s (%2$s) with %3$s"),
	                   d.c_str(), sizeString().c_str(), fsTypeString().c_str() );
	    }
	else if( mp==bootMount() )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    // %3$s is replaced by file system type (e.g. reiserfs)
	    txt = sformat( _("Create boot partition %1$s (%2$s) with %3$s"),
	                   d.c_str(), sizeString().c_str(), fsTypeString().c_str() );
	    }
	else if( !mp.empty() )
	    {
	    if( encryption==ENC_NONE )
		{
		// displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		// %3$s is replaced by file system type (e.g. reiserfs)
		// %4$s is replaced by mount point (e.g. /usr)
		txt = sformat( _("Create partition %1$s (%2$s) for %4$s with %3$s"),
			       d.c_str(), sizeString().c_str(), fsTypeString().c_str(),
			       mp.c_str() );
		}
	    else
		{
		// displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		// %3$s is replaced by file system type (e.g. reiserfs)
		// %4$s is replaced by mount point (e.g. /usr)
		txt = sformat( _("Create encrypted partition %1$s (%2$s) for %4$s with %3$s"),
			       d.c_str(), sizeString().c_str(), fsTypeString().c_str(),
			       mp.c_str() );
		}
	    }
	else if( idt != ID_SWAP && idt != ID_LINUX && idt<256 )
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    // %3$s is replaced by hexadecimal number (e.g. 8E)
	    txt = sformat( _("Create partition %1$s (%2$s) with id=%3$X"),
			   d.c_str(), sizeString().c_str(), idt );
	    }
	else
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Create partition %1$s (%2$s)"),
			   d.c_str(), sizeString().c_str() );
	    }
	}
    return( txt );
    }

string Partition::formatText( bool doing ) const
    {
    string txt;
    string d = dev;
    if( doing )
	{
	// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
	// %2$s is replaced by size (e.g. 623.5 MB)
	// %3$s is replaced by file system type (e.g. reiserfs)
	txt = sformat( _("Formatting partition %1$s (%2$s) with %3$s "),
		       d.c_str(), sizeString().c_str(), fsTypeString().c_str() );
	}
    else
	{
	if( !mp.empty() )
	    {
	    if( mp=="swap" )
		{
		// displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		txt = sformat( _("Format partition %1$s (%2$s) for swap"),
			       d.c_str(), sizeString().c_str() );
		}
	    else if( encryption==ENC_NONE )
		{
		// displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		// %3$s is replaced by file system type (e.g. reiserfs)
		// %4$s is replaced by mount point (e.g. /usr)
		txt = sformat( _("Format partition %1$s (%2$s) for %4$s with %3$s"),
			       d.c_str(), sizeString().c_str(), fsTypeString().c_str(),
			       mp.c_str() );
		}
	    else
		{
		// displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		// %3$s is replaced by file system type (e.g. reiserfs)
		// %4$s is replaced by mount point (e.g. /usr)
		txt = sformat( _("Format encrypted partition %1$s (%2$s) for %4$s with %3$s"),
			       d.c_str(), sizeString().c_str(), fsTypeString().c_str(),
			       mp.c_str() );
		}
	    }
	else
	    {
	    // displayed text before action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    // %3$s is replaced by file system type (e.g. reiserfs)
	    txt = sformat( _("Format partition %1$s (%2$s) with %3$s"),
			   d.c_str(), sizeString().c_str(), fsTypeString().c_str() );
	    }
	}
    return( txt );
    }

string Partition::resizeText( bool doing ) const
    {
    string txt;
    string d = dev;
    if( doing )
        {
	if( needShrink() )
	    // displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Shrinking partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
	else
	    // displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
	    // %2$s is replaced by size (e.g. 623.5 MB)
	    txt = sformat( _("Extending partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
	}
    else
        {
	if( isWindows() )
	    {
	    if( needShrink() )
		// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		txt = sformat( _("Shrink Windows partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
	    else
		// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		txt = sformat( _("Extend Windows partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
    	    }
        else
            {
	    if( needShrink() )
		// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		txt = sformat( _("Shrink partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
	    else
		// displayed text during action, %1$s is replaced by device name e.g. /dev/hda1
		// %2$s is replaced by size (e.g. 623.5 MB)
		txt = sformat( _("Extend partition %1$s to %2$s"), d.c_str(), sizeString().c_str() );
	    }
	}
    return( txt );
    }

void Partition::getCommitActions( list<commitAction*>& l ) const
    {
    unsigned s = l.size();
    bool change_id = idt!=orig_id;
    Volume::getCommitActions( l );
    if( s<l.size() && change_id )
        {
	list<commitAction*>::iterator last = l.end();
	--last;
	if( (*last)->stage>MOUNT )
	    {
	    delete( *last );
	    l.erase( last );
	    }
	else
	    change_id = false;
	}
    if( change_id )
	{
	l.push_back( new commitAction( INCREASE, cont->staticType(),
				       setTypeText(false), false ));
	}
    }

Partition::~Partition()
    {
    y2milestone( "destructed partition %s", dev.c_str() );
    }

void
Partition::getInfo( PartitionInfo& tinfo ) const
    {
    info.partitionType = type ();
    info.cylStart = cylStart ();
    info.cylSize = cylSize ();
    info.nr = num;
    info.id = idt;
    info.boot = bootflag;
    tinfo = info;
    }

namespace storage
{

std::ostream& operator<< (std::ostream& s, const Partition &p )
    {
    s << "Partition " << *(Volume*)&p
      << " Start:" << p.reg.start()
      << " CylNum:" << p.reg.len()
      << " Id:" << std::hex << p.idt << std::dec;
    if( p.typ!=storage::PRIMARY )
      s << " " << Partition::pt_names[p.typ];
    if( p.orig_num!=p.num )
      s << " OrigNr:" << p.orig_num;
    if( p.orig_id!=p.idt )
      s << " OrigId:" << p.orig_id;
    if( p.bootflag )
      s << " boot";
    return( s );
    }

}

void Partition::logDifference( const Partition& rhs ) const
    {
    string log = Volume::logDifference( rhs );
    if( reg.start()!=rhs.reg.start() )
	log += " Start:" + decString(reg.start()) + "-->" + 
	       decString(rhs.reg.start());
    if( reg.len()!=rhs.reg.len() )
	log += " CylNum:" + decString(reg.len()) + "-->" + 
	       decString(rhs.reg.len());
    if( typ!=rhs.typ )
	log += " Typ:" + pt_names[typ] + "-->" + pt_names[rhs.typ];
    if( idt!=rhs.idt )
	log += " Id:" + hexString(idt) + "-->" + hexString(rhs.idt);
    if( orig_id!=rhs.orig_id )
	log += " OrigId:" + hexString(orig_id) + "-->" + hexString(rhs.orig_id);
    if( orig_num!=rhs.orig_num )
	log += " OrigNr:" + decString(orig_num) + "-->" + decString(rhs.orig_num);
    if( bootflag!=rhs.bootflag )
	{
	if( rhs.bootflag )
	    log += " -->boot";
	else
	    log += " boot-->";
	}
    y2milestone( "%s", log.c_str() );
    }

bool Partition::equalContent( const Partition& rhs ) const
    {
    return( Volume::equalContent(rhs) &&
            reg==rhs.reg && bootflag==rhs.bootflag && typ==rhs.typ && 
            idt==rhs.idt );
    }

Partition& Partition::operator= ( const Partition& rhs )
    {
    y2milestone( "operator= from %s", rhs.nm.c_str() );
    *((Volume*)this) = rhs;
    reg = rhs.reg;
    bootflag = rhs.bootflag;
    typ = rhs.typ;
    idt = rhs.idt;
    orig_id = rhs.orig_id;
    orig_num = rhs.orig_num;
    return( *this );
    }

Partition::Partition( const Disk& d, const Partition& rhs ) : Volume(d)
    {
    y2milestone( "constructed partition by copy constructor from %s", 
                 rhs.dev.c_str() );
    *this = rhs;
    }

string Partition::pt_names[] = { "primary", "extended", "logical", "any" };

