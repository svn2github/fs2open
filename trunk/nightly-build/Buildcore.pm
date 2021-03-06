package Buildcore;

# Buildcore plugin for nightly and release build system - common components 1.1.0
# 1.1.0 - Add SHA-256 output and upload for all files within a build, for the Installer.
# 1.0.1 - MSVC 201x support
# 1.0.0 - Initial release

use strict;
use warnings;

use Cwd;
use Config::Tiny;
use Net::FTP;
#use Net::SFTP::Foreign;
use File::Copy;
use File::Copy::Recursive qw(rcopy);
use File::Path;
use File::Spec::Functions;
use File::Basename;
use lib dirname (__FILE__);
use Digest::MD5 qw(md5_hex);
use Digest::SHA;
use feature 'switch';

my $CONFIG = Config::Tiny->new();
$CONFIG = Config::Tiny->read(dirname (__FILE__) . "/buildconfig.conf"); # Read in the plugin config info
if(!(Config::Tiny->errstr() eq "")) { die "Could not read config file, did you copy the sample to buildconfig.conf and edit it?\n"; }
my $OS = getOS();

my $basename_suffix = '';
my $version = '';
my %archives;
my %dmgs;
my %md5s;
my %shas;
my %installer_hashes;

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
	elsif($^O eq "freebsd") {
		return "FREEBSD";
	}
	else {
		return 0;
	}
}

sub set_basename_suffix
{
	$basename_suffix = shift;
}

sub set_version
{
	$version = shift;
}

sub get_archives
{
	return %archives;
}

sub get_dmgs
{
	return %dmgs;
}

sub get_md5s
{
	return %md5s;
}

sub get_shas
{
	return %shas;
}

sub get_mode
{
	return ($version ne '') ? 'release' : 'nightly';
}

sub make_dmg
{
	return ($OS eq 'OSX' && get_mode() eq 'release');
}

sub compile
{
	my $exportpath = shift;

	my @filenames;
	my @outputlist;
	my $output;
	my $command;
	my $cleancmd;
	my $currentdir;
	my $group;
	my $config;
	my %CONFIG_NAMES;
	my %CONFIG_STRINGS;
	my %BUILD_CONFIGS;
	@CONFIG_NAMES{split(/~/, $CONFIG->{$OS}->{config_groups})} = split(/::/, $CONFIG->{$OS}->{config_names});
	@CONFIG_STRINGS{split(/~/, $CONFIG->{$OS}->{config_groups})} = split(/::/, $CONFIG->{$OS}->{config_strings});

#	@BUILD_CONFIGS{split(/~/, $CONFIG->{$OS}->{config_names})} = split(/~/, $CONFIG->{$OS}->{config_strings});

	$currentdir = cwd();

	unless(-d $exportpath)
	{
		print STDERR "Could not find source code at " . $exportpath . "\n";
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

		foreach $config (keys (%BUILD_CONFIGS))
		{
			print "Building " . $config . "...\n";

			if($OS eq "WIN") {
				if($CONFIG->{$OS}->{compiler} eq "MSVC2008") {
					$command = $CONFIG->{$OS}->{build_program_path} . " /nocolor /nologo /rebuild Freespace2.sln \"" . $BUILD_CONFIGS{$config} . "\"";
				}
				elsif($CONFIG->{$OS}->{compiler} eq "MSVC6") {
					$command = $CONFIG->{$OS}->{build_program_path} . " Freespace2.dsw /MAKE \"Freespace2 - " . $BUILD_CONFIGS{$config} . "\" /MAKE \"Fred2 - " . $BUILD_CONFIGS{$config} . "\" /REBUILD";
				}
				elsif($CONFIG->{$OS}->{compiler} eq "MSVC201x") {
					$command = $CONFIG->{$OS}->{build_program_path} . " Freespace2.sln /t:Rebuild /p:Configuration=\"" . $BUILD_CONFIGS{$config} . "\"";
				}
				else {
					# Compiler flag not set correctly
					print STDERR "Unrecognized compiler setting, must be one of:  MSVC201x MSVC2008 MSVC6\n";
					return 0;
				}
			}
			elsif($OS eq "OSX") {
				$command = $CONFIG->{$OS}->{build_program_path} . " -project FS2_Open.xcodeproj -configuration " . $BUILD_CONFIGS{$config} . " clean build";
			}
			elsif($OS eq "LINUX" || $OS eq "FREEBSD") {
				$command = "./autogen.sh " . $BUILD_CONFIGS{$config} . " 2>&1 && " . $CONFIG->{$OS}->{build_program_path} . " clean 2>&1 && " . $CONFIG->{$OS}->{build_program_path};
			}
			else {
				# How the fuck did you get this far with an unrecognized OS
				print STDERR "Unrecognized OS detected.\n";
				return 0;
			}

			$command = $command . " 2>&1";
			print $command . "\n";
			$output = `$command`;

			push(@outputlist, $output);

			# TODO:  Check @outputlist for actual changes, or if it just exited without doing anything
			if(($OS eq "OSX" && $output =~ /\ BUILD\ FAILED\ /) ||
				($OS eq "WIN" && !($output =~ /0\ Projects\ failed/)) ||
				(($OS eq "LINUX" || $OS eq "FREEBSD") && $output =~ /\ Error\ 1\n$/)) {
				print $output . "\n\n";
				print STDERR "Building " . $config . " failed, see output for more information.\n";
				return 0;
			}
			push(@filenames, move_and_rename($exportpath, $config));
		}

		($archives{$group}, $dmgs{$group}, $md5s{$group}, $shas{$group}) = archive($group, @filenames);
	}

	chdir($currentdir);

	return 1;
}

sub move_and_rename
{
	my $exportpath = shift;
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
		if($_ =~ /^$CONFIG->{$OS}->{output_filename}$/)
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

	foreach $oldname (@files)
	{
		$foundext = "";
		$basename = $oldname;
		if($ext ne "")
		{
			$basename =~ s/(${ext})$//;
			$foundext = $1;
		}

		$newname = $basename . $basename_suffix . $foundext;

		push(@returnfiles, $newname);
		print "Moving " . $this_build_drop . "/" . $oldname . " to " . $CONFIG->{$OS}->{archive_path} . "/" . $newname . "\n";
		move(catfile($this_build_drop, $oldname), catfile($CONFIG->{$OS}->{archive_path}, $newname));
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
	my $dmgcommand;
	my $dmgoutput;
	my $var_prefix;
	my $basename;
	my $archivename;
	my $dmgname = '';
	my $md5name;
	my $shaname = '';
	my $srcfolder = 'FS2_Open ' . $version . ' ' . $group;
	my $dirversion = $version;
	$dirversion =~ s/\ /_/g;
	my $currentdir = cwd();

	if($#filenames == -1)
	{
		die "No filenames to archive, terminating...\n";
	}

	chdir($CONFIG->{$OS}->{archive_path});

	if(make_dmg())
	{
		mkdir($srcfolder);
	}

	%installer_hashes = ();
	foreach (@filenames)
	{
		unless(-s $_)
		{
			die "File " . $_ . " does not exist, terminating...\n";
		}

		$args .= " \"" . $_ . "\"";

		if(get_mode() eq 'release')
		{
			# Get the hashes now, before moving any files.
			processfiles($_);
		}

		if(make_dmg())
		{
			rcopy($_, catfile($srcfolder, $_));
		}
	}

	if($args eq "")
	{
		print STDERR "Empty arguments for archiving, terminating...\n";
		return ('', '');
	}

	unless(get_mode() eq 'release')
	{
		$basename = "fso_" . $group . $basename_suffix;
	}
	else
	{
		$basename = 'fs2_open_' . $dirversion;
		$basename .= ($group ne 'Standard') ? '_' . $group : '';
		# Dump the hashes we got earlier to a file in the archive location.
		$shaname = dumphashes($basename);
	}

	$archivename = $basename . $CONFIG->{$OS}->{get_mode() . '_archive_ext'};
	$command = $CONFIG->{$OS}->{get_mode() . '_path_to_archiver'} . " " . $CONFIG->{$OS}->{get_mode() . '_archiver_args'} . " " . $archivename . $args;

	if(make_dmg())
	{
		$dmgname = $basename . '.dmg';
		$dmgcommand = 'hdiutil create -srcfolder "' . $srcfolder . '" ' . $dmgname;
	}

	$md5name = $basename . '.md5';
	$command2 = $CONFIG->{$OS}->{md5} . " " . $archivename . (make_dmg() ? " " . $dmgname : "") . " > " . $md5name;

	# archive the files
	if(make_dmg())
	{
		print "Executing:  " . $dmgcommand . "\n";
		$dmgoutput = `$dmgcommand 2>&1`;
#		print $dmgoutput . "\n";
	}
	print "Executing:  " . $command . "\n";
	$output = `$command 2>&1`;
#	print $output . "\n";
	print "Executing:  " . $command2 . "\n";
	#md5 the archive
	$output2 = `$command2 2>&1`;
#	print $output2 . "\n";

	if(make_dmg() && -d $srcfolder)
	{
		rmtree($srcfolder);
	}

	foreach (@filenames)
	{
		if(-e $_)
		{
			if(-f $_)
			{
				unlink $_ or print STDERR "Could not unlink $_: $!";
			}
			elsif(-d $_)
			{
				rmtree($_);
			}
		}
		else
		{
			print STDERR "Was told $_ does not exist for removal.";
		}
	}

	chdir($currentdir);

	return ($archivename, $dmgname, $md5name, $shaname);
}

sub processfiles
{
	my $dirname = shift;

	if(-f $dirname)
	{
		# Is file, just sha-256 the file
		$installer_hashes{$dirname} = getchecksum($dirname);
		return 0;
	}

	opendir(DIRH, $dirname);

	#
	# Read all the files and directories excluding the current
	# '.' and parent directory '..'  
	#

	my @files = sort (grep { !/^\.|\.\.}$/ } readdir (DIRH));
	closedir(DIRH);
	my $file;
	foreach $file (@files)
	{
		my $fullpath = $dirname . "/" . $file;
		if (-d $fullpath)
		{
			processfiles($fullpath);
		}
		else
		{
			$installer_hashes{$fullpath} = getchecksum($fullpath);
		}
	}

	return 0;
}

sub getchecksum
{
	my $file = shift;
	my $hash = Digest::SHA->new(256);
	if (! -r $file)
	{
		die "Could not read $file for hashing, terminating.\n";
	}
	open (my $fh, "< :raw", $file) or die "Could not open $file for hashing, terminating.\n";
	$hash->addfile($fh, "b");
	close($fh);
	return $hash->hexdigest;
}

sub dumphashes
{
	my $shafile = $_[0] . '.sums';
	my $file;
	my $hash;

	chdir($CONFIG->{$OS}->{archive_path});

	if (-e $shafile)
	{
		unlink($shafile) or print STDERR "Could not unlink $shafile: $!";
	}

	open(my $fh, ">", $shafile);
	while(($file, $hash) = each %installer_hashes)
	{
		print $fh "HASH\nSHA-256\n$file\n$hash\n";
	}
	close($fh);

	return $shafile;
}

sub upload
{
	# Upload $archivename to the server, use Net::FTP.
	my ($archivename, $dmgname, $md5name, $shaname, $local_path, $upload_path) = @_;
	my $fh;
	my $ftp;
	my $currentdir = cwd();
	my $mirror;
	my %MIRROR_TYPES;
	my %MIRROR_HOSTS;
	my %MIRROR_PORTS;
	my %MIRROR_USERNAMES;
	my %MIRROR_PASSWORDS;
	my %MIRROR_BUILDS_PATHS;
	my @mirrors = split(/~/, $CONFIG->{general}->{get_mode() . '_mirrors'});
	@MIRROR_TYPES{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{types});
	@MIRROR_HOSTS{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{hostnames});
	@MIRROR_PORTS{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{ports});
	@MIRROR_USERNAMES{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{usernames});
	@MIRROR_PASSWORDS{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{passwords});
	@MIRROR_BUILDS_PATHS{split(/~/, $CONFIG->{mirrors}->{names})} = split(/::/, $CONFIG->{mirrors}->{builds_paths});

	chdir($local_path);

	# Should have the full path to both files to upload, so upload them
	# Connection info is in $CONFIG, read from config file.

	if(!(-e $archivename && -s $archivename && -e $md5name && -s $md5name))
	{
		die "Could not find a file to upload, terminating...\n";
	}

	foreach $mirror (@mirrors)
	{
		print "Uploading " . $local_path . "/" . $archivename . ( ($dmgname ne '') ? (" and " . $local_path . "/" . $dmgname) : '' ) . " and " . $local_path . "/" . $md5name . ( ($shaname ne '') ? (" and " . $local_path . "/" . $shaname) : '' ) . " to " . $MIRROR_HOSTS{$mirror} . "\n";

		given ($MIRROR_TYPES{$mirror})
		{
			when (/^ftp$/)
			{
				$ftp = Net::FTP->new($MIRROR_HOSTS{$mirror}, port => $MIRROR_PORTS{$mirror}, Debug => 0, Passive => 1) or die "Cannot connect to " . $MIRROR_HOSTS{$mirror} . ": $@";
				$ftp->login($MIRROR_USERNAMES{$mirror}, $MIRROR_PASSWORDS{$mirror}) or die "Cannot login ", $ftp->message;
				$ftp->cwd($MIRROR_BUILDS_PATHS{$mirror} . "/" . $upload_path) or die "Cannot change working directory ", $ftp->message;
				$ftp->binary();
				$ftp->put($archivename) or die "Upload of archive failed ", $ftp->message;
				if($dmgname ne '')
				{
					$ftp->put($dmgname) or die "Upload of DMG archive failed ", $ftp->message;
				}
				$ftp->ascii();
				$ftp->put($md5name) or die "Upload of md5sum failed ", $ftp->message;
				if($shaname ne '')
				{
					$ftp->put($shaname) or die "Upload of SHA sum failed ", $ftp->message;
				}
				$ftp->quit;
			}
			when (/^sftp$/)
			{
				# Not currently functioning on Windows with password auth
#				$ftp = Net::SFTP::Foreign->new($MIRROR_HOSTS{$mirror}, port => $MIRROR_PORTS{$mirror}, user => $MIRROR_USERNAMES{$mirror}, password => $MIRROR_PASSWORDS{$mirror});
#				$ftp->error and die "Something bad happened connecting to " . $MIRROR_HOSTS{$mirror} . ": " . $ftp->error;
#				$ftp->setcwd($MIRROR_BUILDS_PATHS{$mirror} . "/" . $upload_path);
#				$ftp->put($archivename, $archivename);
#				if($dmgname ne '')
#				{
#					$ftp->put($dmgname, $dmgname);
#				}
#				$ftp->put($md5name, $md5name);
#				if($shaname ne '')
#				{
#					$ftp->put($shaname, $shaname);
#				}
#				$ftp->disconnect();

				# Write certs to a file, upload file, remove file.  Sync script on primary FTP will finish distribution to mirrors.
				open($fh, ">", md5_hex($mirror) . '.scert') or die "cannot open > " . md5_hex($mirror) . ".scert: $!";;
				print($fh $MIRROR_PASSWORDS{$mirror} . "\n");
				close($fh);
				$ftp = Net::FTP->new($MIRROR_HOSTS{$CONFIG->{general}->{release_primary}}, port => $MIRROR_PORTS{$CONFIG->{general}->{release_primary}}, Debug => 0, Passive => 1) or die "Cannot connect to " . $MIRROR_HOSTS{$CONFIG->{general}->{release_primary}} . ": $@";
				$ftp->login($MIRROR_USERNAMES{$CONFIG->{general}->{release_primary}}, $MIRROR_PASSWORDS{$CONFIG->{general}->{release_primary}}) or die "Cannot login ", $ftp->message;
				$ftp->ascii();
				$ftp->put(md5_hex($mirror) . '.scert') or die "Upload of scert failed ", $ftp->message;
				$ftp->quit;
				unlink(md5_hex($mirror) . '.scert')  or print STDERR "Could not unlink the scert file: $!";
			}
		}
	}

	chdir($currentdir);

	return 1;
}

1;
