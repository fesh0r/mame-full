#!/usr/bin/perl

###########################################################################
# Nathan Hand (nathanh@manu.org.au) (c) May 2000
#
# redistribution under the GNU GPL v2 is permitted.
#
# simple utility to analyze a split romset built around zipfiles. doesn't
# support any other method yet (directories, merged sets). unlikely to be
# maintained. might be useful to developers of other romset analyzers. in
# particular, i think the method used here to extract romset info and get
# a high level storage of the data is really smick.
#
# how to use it? stick all your roms in zipfiles, using split romsets, so
# that (for example) pacman plus two clones would be like this
#
#     (dir)/pacman.zip         - all roms for pacman original
#     (dir)/pacmod.zip         - all roms in the pacmod clone
#     (dir)/pacmanjp.zip       - all roms in the pacmanjp clone
#
# the terminology i use is that pacman is your "master" set, while pacmod
# and pacmanjp are the "clone" sets. also pacmod is a "cousin" set of the
# pacmanjp set. all the files that comprise the set are called "roms". my
# script relies on the following layout.
#
#     (1) master zips contain master roms
#     (2) clone zips contain clone roms
#     (3) roms common to 2 or more cousins go in the master set
#
# the script reads each zipfile, makes sure all the roms are in the right
# zipfiles, looks for extra files that shouldn't be there, etc.
#
# i've used this script to cleanup my sets. it found a number of errors i
# didn't find with -verifyroms or other rom analyzers.
###########################################################################

# config options, nothing fancy

$quiet = 1;
$zipdir = "/usr/lib/games/xmame/roms";
$unzip_binary = "unzip";
$xmame_binary = "xmame";

# examine zipfiles, extract fileinfo

opendir(DIR, $zipdir) || die "cannont opendir $zipdir: $!";
@zipfiles = grep { -f "$zipdir/$_" } readdir(DIR);
closedir(DIR);

foreach $zipfile (@zipfiles) {
        foreach $file (`$unzip_binary -v -qq $zipdir/$zipfile`) {
                $file =~ tr/A-Z/a-z/;
                @file = split(/\s+/, $file);

                $zipfile =~ tr/A-Z/a-z/;
                $zipfile =~ s/\.zip$//;

                ($dum, $len, $met, $siz, $rat, $dat, $tim, $crc, $nam) = @file;
                push(@{$zips{$zipfile}}, "( name $nam size $len crc $crc )");
        }
}

# examine listinfo, extract listinfo

foreach $line (`$xmame_binary -listinfo`) {
        chop $line;

        if ($line eq "game (" || $line eq "resource (") {
                undef $name;
        }

        $name = $1                     if ($line =~ /^\s+name\s(.*)$/);
        $description{$name} = $1       if ($line =~ /^\s+description\s(.*)$/);
        $year{$name} = $1              if ($line =~ /^\s+year\s(.*)$/);
        $manufacturer{$name} = $1      if ($line =~ /^\s+manufacturer\s(.*)$/);
        $history{$name} = $1           if ($line =~ /^\s+history\s(.*)$/);
        $video{$name} = $1             if ($line =~ /^\s+video\s(.*)$/);
        $sound{$name} = $1             if ($line =~ /^\s+sound\s(.*)$/);
        $input{$name} = $1             if ($line =~ /^\s+input\s(.*)$/);
        $driver{$name} = $1            if ($line =~ /^\s+driver\s(.*)$/);
        $master{$name} = $1            if ($line =~ /^\s+cloneof\s(.*)$/);
        $master{$name} = $1            if ($line =~ /^\s+romof\s(.*)$/);

        push(@{$roms{$name}}, $1)         if ($line =~ /^\s+rom\s(.*)$/);
        push(@{$chips{$name}}, $1)        if ($line =~ /^\s+chip\s(.*)$/);
        push(@{$dipswitches{$name}}, $1)  if ($line =~ /^\s+dipswitch\s(.*)$/);

        if ($line eq ")") {
                print "processed $name\n" unless $quiet;
        }
}

# strip merge info from all roms in all sets

foreach $set (keys %description) {
        @{$roms{$set}} = map {s/merge\s\S+\s//; $_} @{$roms{$set}};
}

# strip master roms from clone sets

foreach $clone (keys %master) {
        local(%mark);
        grep($mark{$_}++, @{$roms{$master{$clone}}});
        @{$roms{$clone}} = grep(!$mark{$_}, @{$roms{$clone}});
}

# move shared roms from clones to master
#
#   step 1 - find cousins of clone, build list of common roms
#   step 2 - strip common roms from cousins and clone
#   step 3 - add common roms to master

foreach $clone (keys %master) {
        local(@common_roms);

        foreach $cousin (keys %description) {
                next if $clone eq $cousin;
                next if $master{$clone} ne $master{$cousin};

                local(%mark);
                grep($mark{$_}++, @{$roms{$cousin}});
                push(@common_roms, grep($mark{$_}, @{$roms{$clone}}));
        }

        foreach $cousin (keys %description) {
                next if $master{$clone} ne $master{$cousin};

                local(%mark);
                grep($mark{$_}++, @common_roms);
                @{$roms{$cousin}} = grep(!$mark{$_}, @{$roms{$cousin}});
        }

        local(%mark);
        grep($mark{$_}++, @common_roms);
        push(@{$roms{$master{$clone}}}, keys(%mark));
}

# info received and mangled, time to analyse

foreach $set (keys %description) {
        local(%mark);
        grep($mark{$_}++, @{$zips{$set}});
        @missing_roms = grep(!$mark{$_}, @{$roms{$set}});

        local(%mark);
        grep($mark{$_}++, @{$roms{$set}});
        @extra_roms = grep(!$mark{$_}, @{$zips{$set}});

        print "examining romset $set... ";
        print "broken!\n" if scalar(@missing_roms) || scalar(@extra_roms);
        print "perfect!\n" unless scalar(@missing_roms) || scalar(@extra_roms);

        foreach $rom (@missing_roms) {
                print "\tmissing rom $rom\n";
        }
        foreach $rom (@extra_roms) {
                print "\textra rom $rom\n";
        }
}
