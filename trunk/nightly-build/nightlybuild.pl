#!/usr/bin/perl -W

# Nightly build script version 1.6.0
# 1.6.0 - Added stoprevision command line support, shifted around some debug output
# 1.5.1 - Fix problems with VS2008, to allow spaces in config names.
# 1.5.0 - Big update to allow for building more configs and grouping them into different archives, all in the same post.
# 1.4.4 - Grab the VC2008 pdb files by changing the regex in the config
# 1.4.3 - fix a move issue, use the config name for the folder and not the config string
# 1.4.2 - fix some more issues with the export method
# 1.4.1 - just a bit more cleanup
# 1.4 - performs a local export before compiling for clean working dir, also checks Linux build output for error
# 1.3.1 - checks for most directories instead of assuming they exist
# 1.3 - cleans builds every compile, supports VC6 as well as VC2008, set up options in config file
# 1.2 - added a lot of error checking, mostly making sure files exist at every step, does some output parsing for error statements
# 1.1 - support for Linux (make) and OS X (xcodebuild), switched to ini style config
# 1.0 - Support for Windows via MSVC2008

use strict;
# Switch top line to -w for production, -W for testing

use Config::Tiny;
use File::Copy;
use Net::FTP;
use Smf;
use Cwd;
use Data::Dumper;
use File::Path;
use Getopt::Long;

my $CONFIG = Config::Tiny->new();
$CONFIG = Config::Tiny->read("buildconfig.conf"); # Read in the ftp and forum authentication info
my $OS = getOS();
if(!$OS) { die "Unrecognized OS.\n"; }

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;
my @months = qw(01 02 03 04 05 06 07 08 09 10 11 12);
my @abbr = qw( Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec );
$year += 1900;
my $monthword = $abbr[$mon];
$mon = $months[$mon];
if($mday < 10)
{
	$mday = "0" . $mday;
}
my $DATE = $year . $mon . $mday;

my @filenames;
my $oldrevision;
my $revision;
my %archives;
my %md5s;
my @archiveslist;
my $exportpath;
my $stoprevision = '';

GetOptions ('stoprevision:s' => \$stoprevision);

if(updatesvn() != 1)
{
	if($revision eq "FAILURE")
	{
		die "Error checking for a revision change, terminating.\n";
	}
	else
	{
		die "SVN still at revision " . $revision . ", terminating.\n";
	}
}

print "SVN has been updated to revision " . $revision . ", compiling...\n";

if(export() != 1)
{
	die "Export to " . $exportpath . " failed...\n";
}

# call Compile scripts
if(compile() != 1)
{
	die "Compile failed...\n";
}

print "Compiling completed\n";
# Code was compiled and updated, move the built files somewhere for archiving
rmtree($exportpath);

foreach (keys (%archives))
{
	upload($archives{$_}, $md5s{$_});
}

post();


#############SUBROUTINES################


sub getOS
{
	if($^O eq "MSWin32") {
		return "WIN";
	}
	elsif($^O eq "linux") {
		return "LINUX";
	}
	elsif($^O eq "darwin" || $^O eq "rhapsody") {
		return "OSX";
	}
	else {
		return 0;
	}
}

sub getrevision
{
	my $command;
	my $output;
	
	$command = "svnversion -n " . $CONFIG->{$OS}->{source_path};
	$output = `$command 2>&1`;
	
	return $output;
}

sub updatesvn
{
	my $rline = '';
	my $updateoutput;
	if($stoprevision)
	{
		$rline = '-r ' . $stoprevision . ' ';
	}
	my $updatecommand = "svn update " . $rline . $CONFIG->{$OS}->{source_path};
	
	unless(-d $CONFIG->{$OS}->{source_path})
	{
		print "Could not find source code at " . $CONFIG->{$OS}->{source_path} . "\n";
		$revision = "FAILURE";
		return 0;
	}
	
	$oldrevision = getrevision();
	
	$updateoutput = `$updatecommand`;
#	$updateoutput = "Updated to revision 4823.";

	# print $updateoutput;
	
	# TODO:  Simple test for now.  Later, if not ^At revision, filter out project file updates that don't apply and other unimportant changes.
	if($updateoutput =~ /^At revision /)
	{
		# No change to source
		$updateoutput =~ /At revision (\d*)\./;
		$revision = $1;
		return 0;
	}
	elsif($updateoutput =~ /Updated to revision /)
	{
		# Source has changed
		$updateoutput =~ /Updated to revision (\d*)\./;
		$revision = $1;
		return 1;
	}
	else
	{
		# Unexpected data received
		print "Unrecognized data:\n".$updateoutput."\n";
		$revision = "FAILURE";
		return 0;
	}
}

sub export
{
	my $exportoutput;
	my $exportcommand;
	my $i = 0;

	do {
		$exportpath = $CONFIG->{$OS}->{source_path}."_".$i++;
	} while (-d $exportpath);

	print "Going to export ".$CONFIG->{$OS}->{source_path}." to directory ".$exportpath."\n";

	$exportcommand = "svn export " . $CONFIG->{$OS}->{source_path} . " " . $exportpath;
#	print $exportcommand . "\n";
	$exportoutput = `$exportcommand`;
	
	if($exportoutput =~ /^Export complete./)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

sub compile
{
	my @outputlist;
	my $output;
	my $command;
	my $cleancmd;
	my $currentdir;
	my $group;
	my %CONFIG_NAMES;
	my %CONFIG_STRINGS;
	my %BUILD_CONFIGS;
	@CONFIG_NAMES{split(/~/, $CONFIG->{$OS}->{config_groups})} = split(/::/, $CONFIG->{$OS}->{config_names});
	@CONFIG_STRINGS{split(/~/, $CONFIG->{$OS}->{config_groups})} = split(/::/, $CONFIG->{$OS}->{config_strings});

#	@BUILD_CONFIGS{split(/~/, $CONFIG->{$OS}->{config_names})} = split(/~/, $CONFIG->{$OS}->{config_strings});
	
	$currentdir = cwd();

	unless(-d $exportpath)
	{
		print "Could not find source code at " . $exportpath . "\n";
		return 0;
	}
	
	chdir($exportpath);
	if($OS eq "OSX" || $OS eq "WIN")
	{
		chdir("projects/" . $CONFIG->{$OS}->{project} . "/");
		if($OS eq "OSX")
		{
			`tar -xzf Frameworks.tgz`;
		}
	}
	
	foreach $group (keys (%CONFIG_NAMES))
	{
		@filenames = ();
		%BUILD_CONFIGS = ();
		@BUILD_CONFIGS{split(/~/, $CONFIG_NAMES{$group})} = split(/~/, $CONFIG_STRINGS{$group});
		
		foreach(keys (%BUILD_CONFIGS))
		{
			print "Building " . $_ . "...\n";
			
			if($OS eq "WIN") {
				if($CONFIG->{$OS}->{compiler} eq "MSVC2008") {
					$command = $CONFIG->{$OS}->{build_program_path} . " /nocolor /nologo /rebuild Freespace2.sln \"" . $BUILD_CONFIGS{$_} . "\"";
				}
				elsif($CONFIG->{$OS}->{compiler} eq "MSVC6") {
					$command = $CONFIG->{$OS}->{build_program_path} . " Freespace2.dsw /MAKE \"Freespace2 - " . $BUILD_CONFIGS{$_} . "\" /MAKE \"Fred2 - " . $BUILD_CONFIGS{$_} . "\" /REBUILD";
				}
				else {
					# Compiler flag not set correctly
					print "Unrecognized compiler setting, must be one of:  MSVC2008 MSVC6\n";
					return 0;
				}
			}
			elsif($OS eq "OSX") {
				$command = $CONFIG->{$OS}->{build_program_path} . " -project FS2_Open.xcodeproj -configuration " . $BUILD_CONFIGS{$_} . " clean build";
			}
			elsif($OS eq "LINUX") {
				$command = "./autogen.sh " . $BUILD_CONFIGS{$_} . " 2>&1 && " . $CONFIG->{$OS}->{build_program_path} . " clean 2>&1 && " . $CONFIG->{$OS}->{build_program_path};
			}
			else {
				# How the fuck did you get this far with an unrecognized OS
				print "Unrecognized OS detected.\n";
				return 0;
			}
			
			$command = $command . " 2>&1";
			print $command . "\n";
			$output = `$command`;
			
			push(@outputlist, $output);
			
			# TODO:  Check @outputlist for actual changes, or if it just exited without doing anything
			if(($OS eq "OSX" && $output =~ / BUILD FAILED /) || 
				($OS eq "WIN" && !($output =~ /0 Projects failed/)) || 
				($OS eq "LINUX" && $output =~ / Error 1\n$/)) {
				print $output . "\n\n";
				print "Building " . $_ . " failed, see output for more information.\n";
				return 0;
			}
			push(@filenames, move_and_rename($_));
		}
		
		($archives{$group}, $md5s{$group}) = archive($group, @filenames);
	}
	
	chdir($currentdir);
	
	return 1;
}

sub move_and_rename
{
	my $configname = shift;
	my $oldname;
	my $basename;
	my $newname;
	my $command;
	my $foundext;
	my @returnfiles = ();
	my $currentdir = cwd();
	my $this_build_drop = $CONFIG->{$OS}->{build_drop};
	my $ext = $CONFIG->{$OS}->{build_extension};
	$this_build_drop =~ s/##PROJECT##/$CONFIG->{$OS}->{project}/;
	$this_build_drop =~ s/##CONFIG##/$configname/;
	$this_build_drop =~ s/##EXPORTPATH##/$exportpath/;
	
	unless(-d $this_build_drop)
	{
		die "Could not find build drop at " . $this_build_drop . ", terminating.\n";
	}
	
	chdir($this_build_drop);
	my @temp = glob("*");
	my @files;
	foreach (@temp)
	{
		if($_ =~ /$CONFIG->{$OS}->{output_filename}/)
		{
			push(@files, $_);
		}
	}
	
	if($#files == -1)
	{
		die "No files found to move and rename for " . $configname . ", terminating...\n";
	}
	
	chdir($currentdir);
	
	print "Moving and renaming files...\n";

	unless(-d $CONFIG->{$OS}->{archive_path})
	{
		print "Could not find archive path at " . $CONFIG->{$OS}->{archive_path} . ", attempting to create...\n";
		mkpath( $CONFIG->{$OS}->{archive_path}, {verbose => 1} );
		
		unless(-d $CONFIG->{$OS}->{archive_path})
		{
			die "Still could not find archive path at " . $CONFIG->{$OS}->{archive_path} . ", terminating.\n";
		}
	}
	
	foreach (@files)
	{
		$foundext = "";
		$oldname = $_;
		$basename = $oldname;
		if($ext ne "")
		{
			$basename =~ s/(${ext})$//;
			$foundext = $1;
		}
		
		$newname = $basename . "-" . $DATE . "_r" . $revision;
		
		if($foundext ne "")
		{
			$newname .= $foundext;
		}
		
		push(@returnfiles, $newname);
		print "Moving " . $this_build_drop . "/" . $oldname . " to " . $CONFIG->{$OS}->{archive_path} . "/" . $newname . "\n";
		move($this_build_drop . "/" . $oldname, $CONFIG->{$OS}->{archive_path} . "/" . $newname);
	}
	
	return @returnfiles;
}

sub archive
{
	my $group = shift;
	my @filenames = @_;
	my $args = "";
	my $command;
	my $output;
	my $command2;
	my $output2;
	my $basename;
	my $archivename;
	my $md5name;
	my $currentdir = cwd();
	
	if($#filenames == -1)
	{
		die "No filenames to archive, terminating...\n";
	}
	
	chdir($CONFIG->{$OS}->{archive_path});

	foreach (@filenames)
	{
		unless(-e $_ && -s $_)
		{
			die "File " . $_ . " does not exist, terminating...\n";
		}
		
		$args .= " \"" . $_ . "\"";
	}
	
	if($args eq "")
	{
		print "Empty arguments for archiving, terminating...\n";
	}
	
	$basename = "fso-" . $OS . "-" . $group . "-" . $DATE . "_r" . $revision;
	$archivename = $basename . $CONFIG->{$OS}->{archive_ext};
	$command = $CONFIG->{$OS}->{path_to_archiver} . " " . $CONFIG->{$OS}->{archiver_args} . " " . $archivename . $args;
	
	$md5name = $basename . ".md5";
	$command2 = $CONFIG->{$OS}->{md5} . " " . $archivename . " > " . $md5name;
	
	# archive the files
	print "Executing:  " . $command . "\n";
	$output = `$command 2>&1`;
#	print $output . "\n";
	print "Executing:  " . $command2 . "\n";
	#md5 the archive
	$output2 = `$command2 2>&1`;
#	print $output2 . "\n";
	
	foreach (@filenames)
	{
		if(-e $_)
		{
			unlink($_);
		}
	}

	chdir($currentdir);
	
	return ($archivename, $md5name);
}

sub upload
{
	# Upload $archive to the server, use Net::FTP.
	my ($archivename, $md5name) = @_;
	my $ftp;
	my $currentdir = cwd();
	
	chdir($CONFIG->{$OS}->{archive_path});
	
	# Should have the full path to both files to upload, so upload them
	# Connection info is in $CONFIG, read from config file.
	
	if(!(-e $archivename && -s $archivename && -e $md5name && -s $md5name))
	{
		die "Could not find a file to upload, terminating...\n";
	}
	
	print "Uploading " . $CONFIG->{$OS}->{archive_path} . "/" . $archivename . " and " . $CONFIG->{$OS}->{archive_path} . "/" . $md5name . " to " . $CONFIG->{ftp}->{hostname} . "\n";
	
	$ftp = Net::FTP->new($CONFIG->{ftp}->{hostname}, Debug=> 0, Passive=> 1) or die "Cannot connect to ".$CONFIG->{ftp}->{hostname}.": $@";
	$ftp->login($CONFIG->{ftp}->{username}, $CONFIG->{ftp}->{password}) or die "Cannot login ", $ftp->message;
	$ftp->cwd($CONFIG->{general}->{upload_path} . $OS) or die "Cannot change working directory ", $ftp->message;
	$ftp->binary();
	$ftp->put($archivename) or die "Upload of archive failed ", $ftp->message;
	$ftp->ascii();
	$ftp->put($md5name) or die "Upload of md5sum failed ", $ftp->message;
	$ftp->quit;

	chdir($currentdir);
	
	return 1;
}

sub post
{
	my $subject;
	my $message;
	my $logoutput;
	my $logcommand;
	my $archivename;
	my $md5name;

	my $startrevision = $oldrevision + 1;
	
	print "In the post function, submitting post to builds area...\n";
	
	$logcommand = "svn log -v -r " . $startrevision . ":" . $revision . " " . $CONFIG->{$OS}->{source_path};
	$logoutput = `$logcommand`;
	
	# Set up the Subject
	$subject = "Nightly (" . $CONFIG->{$OS}->{os_name} . "): " . $mday . " " . $monthword . " " . $year . " - Revision " . $revision;
	# Set up the message
	$message = "Here is the nightly for " . $CONFIG->{$OS}->{os_name} . " on " . $mday . " " . $monthword . " " . $year . " - Revision " . $revision . "\n\n";
	
	# Make a post on the forums to the download
	foreach (keys (%archives))
	{
		$archivename = $archives{$_};
		$md5name = $md5s{$_};
		$message .= "Group: " . $_ . "\n";
		$message .= "[url=".$CONFIG->{general}->{download_path}.$OS."/".$archivename."]".$archivename."[/url]\n";
		$message .= "[url=".$CONFIG->{general}->{download_path}.$OS."/".$md5name."]MD5Sum[/url]\n";
		$message .= "\n";
	}
		
	$message .= "[code]\n";
	$message .= $logoutput . "\n";
	$message .= "[/code]\n\n";
	
	Smf::set_homeurl($CONFIG->{general}->{forum_url});
	Smf::set_board($CONFIG->{$OS}->{builds_forum});
	Smf::set_username($CONFIG->{forum}->{username});
	Smf::set_password($CONFIG->{forum}->{password});
	Smf::post($subject, $message);
}