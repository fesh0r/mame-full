#!/usr/bin/perl

require 5.000;
use Data::Dumper;

# config options, nothing fancy

$zipdir = "/usr/lib/games/xmame/roms";
$unzip_binary = "unzip";
$xmame_binary = "xmame";

# useful things

sub array_subtract {
        my($ref1, $ref2, %mark) = @_;
        grep $mark{$_}++, @{$ref2};
        return grep !$mark{$_}, @{$ref1};
}

sub array_intersect {
        my($ref1, $ref2, %mark) = @_;
        grep $mark{$_}++, @{$ref2};
        return grep $mark{$_}, @{$ref1};
}

# examine files, extract fileinfo

opendir(DIR, $zipdir) || die "cannont opendir $zipdir: $!";
@files = grep { -f "$zipdir/$_" } readdir(DIR);
closedir(DIR);

foreach $file (@files) {
        foreach $line (`$unzip_binary -v -qq $zipdir/$file`) {
                $line =~ tr/A-Z/a-z/;
                @line = split /\s+/, $line;
                ($dum, $len, $met, $siz, $rat, $dat, $tim, $crc, $nam) = @line;

                $file =~ tr/A-Z/a-z/;
                $file =~ s/\.zip$//;
                $info = "name $nam size $len crc $crc";

                push @{$zips{$file}}, $info;
        }
}

# examine listinfo, extract listinfo

foreach $line (`$xmame_binary -listinfo 2>/dev/null`) {
        chop $line;

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

        if ($line =~ /^\s+rom\s\(\s(.*)\s\)$/) {
                %info = split /\s+/, $1;
                push @{$roms{$name}}, "name $info{name} size $info{size} crc $info{crc}";
        }

        if ($line =~ /^\s+chip\s\(\s(.*)\s\)$/) {
                push @{$chips{$name}}, $1;
        }

        if ($line =~ /^\s+dipswitch\s\(\s(.*)\s\)$/) {
                push @{$dipswitches{$name}}, $1;
        }

        if ($line eq "game (" || $line eq "resource (") {
                undef $rom;
        }
}

# strip master roms from clone sets

foreach $clone (keys %master) {
        @{$roms{$clone}} = &array_subtract($roms{$clone}, $roms{$master{$clone}});
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
                push @common_roms, &array_intersect($roms{$cousin}, $roms{$clone});
        }

        foreach $cousin (keys %description) {
                next if $master{$clone} ne $master{$cousin};
                @{$roms{$cousin}} = &array_subtract($roms{$cousin}, \@common_roms);
        }

        local(%mark);
        grep $mark{$_}++, @common_roms;
        push @{$roms{$master{$clone}}}, keys %mark;
}

# find sets without a corresponding zip, and vice versa

@zipkeys = keys %zips;
@romkeys = keys %roms;

foreach $zip (&array_subtract(\@romkeys, \@zipkeys)) {
        print "file for $zip is missing!\n";
}

foreach $zip (&array_subtract(\@zipkeys, \@romkeys)) {
        print "what is file $zip for?\n";
}

# info received and mangled, time to analyse

foreach $set (keys %description) {
        @missing_roms = &array_subtract($roms{$set}, $zips{$set});
        @extra_roms = &array_subtract($zips{$set}, $roms{$set});

        @misnamed_roms = grep {
                %info = split /\s+/, $rom=$_;
                push @notmissing, grep {
                        $misnamed_be{$rom} = $1 if /name (.*) size $info{size} crc $info{crc}/;
                } @missing_roms;
                $misnamed_be{$rom};
        } @extra_roms;

        @wrongcrc_roms = grep {
                %info = split /\s+/, $rom=$_;
                push @notmissing, grep {
                        $wrongcrc_be{$rom} = $1 if /name $info{name} size $info{size} crc (.*)/;
                } @missing_roms;
                $wrongcrc_be{$rom};
        } @extra_roms;

        @extra_roms = &array_subtract(\@extra_roms, \@misnamed_roms);
        @extra_roms = &array_subtract(\@extra_roms, \@wrongcrc_roms);
        @missing_roms = &array_subtract(\@missing_roms, \@notmissing);

        print "examining $set... ";
        print "broken!\n" if @missing_roms || @extra_roms || @misnamed_roms || @wrongcrc_roms;
        print "perfect!\n" unless @missing_roms || @extra_roms || @misnamed_roms || @wrongcrc_roms;

        foreach $rom (@missing_roms)    { print "\tmissing:   $rom\n"; }
        foreach $rom (@extra_roms)      { print "\textra:     $rom\n"; }
        foreach $rom (@misnamed_roms)   { print "\tmisnamed:  $rom should be $misnamed_be{$rom}\n"; }
        foreach $rom (@wrongcrc_roms)   {
                if ($wrongcrc_be{$rom} eq "00000000") {
                        print "\twrongcrc:  $rom BEST DUMP KNOWN\n";
                } else {
                        print "\twrongcrc:  $rom should be $wrongcrc_be{$rom}\n"
                }
        }
}
