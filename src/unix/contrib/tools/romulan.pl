#!/usr/bin/perl

require 5.000;

# config options, nothing fancy

$quiet = 0;
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
                push(@{$zips{$zipfile}}, "name $nam size $len crc $crc");
        }
}

# examine listinfo, extract listinfo

foreach $line (`$xmame_binary -listinfo 2>/dev/null`) {
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

# strip everything except name, size, crc

foreach $set (keys %description) {
        @{$roms{$set}} = map {s/.*(name\s\S+\ssize\s\S+\scrc\s\S+)\s.*/\1/; $_} @{$roms{$set}};
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

# find sets without a correspondig zip

local(%mark);
grep($mark{$_}++, keys %zips);
@missing_zips = grep(!$mark{$_}, keys %roms);

foreach $zip (@missing_zips) {
        print "zipfile $zip is missing!\n" unless $quiet;
}

# find zips without a corresponding rom

local(%mark);
grep($mark{$_}++, keys %roms);
@extra_zips = grep(!$mark{$_}, keys %zips);

foreach $zip (@extra_zips) {
        print "what is zipfile $zip for?\n" unless $quiet;
}

# info received and mangled, time to analyse

foreach $set (keys %description) {
        local(%mark);
        grep($mark{$_}++, @{$zips{$set}});
        @missing_roms = grep(!$mark{$_}, @{$roms{$set}});

        local(%mark);
        grep($mark{$_}++, @{$roms{$set}});
        @extra_roms = grep(!$mark{$_}, @{$zips{$set}});

        print "examining zipfile $set... " unless $quiet;
        print "broken!\n" if scalar(@missing_roms) || scalar(@extra_roms);
        print "perfect!\n" unless scalar(@missing_roms) || scalar(@extra_roms);

        foreach $rom (@missing_roms) {
                print "\tmissing rom $rom\n" unless $quiet;
        }
        foreach $rom (@extra_roms) {
                print "\textra rom $rom\n" unless $quiet;
        }
}

