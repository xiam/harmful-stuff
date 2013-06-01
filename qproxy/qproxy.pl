#!/usr/bin/perl
# qproxy.pl - a quick 'CONNECT' proxy daemon
# --------------------------------------------------------------------------------
# (c) 2006 by J. Carlos Nieto <xiam@users.sourceforge.net>
# License: http://www.gnu.org/licenses/gpl.txt
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# THANKS:
#
# http://www.web-cache.com/Writings/Internet-Drafts/draft-luotonen-web-proxy-tunneling-01.txt
# http://www.ietf.org/rfc/rfc2616.txt
# http://www.blahz.ab.ca/books/books/perl2/advprog/ch12_03.htm
# http://www.tutorialized.com/tutorial/Reading-binary-data/3399
# $ man perlipc
#
# HOWTO:
#
# Upload this script to an external server you want to use as proxy and run it:
#
# $ chmod +x qproxy.pl
# $ ./qproxy.pl bind_ip proxy_port
#
# By default, qproxy will accept connections ONLY from bind_ip, that's for avoiding
# non authorized use to our proxy without having to use a password. If you don't want
# to see the conversation between server and client you should to redirect it to
# /dev/null.
#
# You may want to use proxytunnel in localhost along with this tool to use services like
# http since proxies for http doesn't use 'CONNECT' method:
#
# $ proxytunnel -a 8080 -d destination_ip:destination_port -p proxy_ip:proxy_port
#

use strict;

use IO::Socket;

sub usage {
  print "$0 [bind IP] [port]\n";
  exit;
};
sub logmsg {
  printf STDOUT "@_\n";
}
sub logdat {
  my $data = shift;
  my $channel = shift;

  my ($line, ) = ($data =~ /^(.*)(\r\n|\n)$/s);
  if (!$line) {
    $line = "data(".length($data).")";
  }

  logmsg $channel.$line;
}

my $bind_addr = shift || usage();
my $bind_port = shift || "8080";

print "* Resolving hostname... ";
my @host = gethostbyname($bind_addr);

if (!@host) {
  print "Not found!.\n";
  exit;
} else {
  $bind_addr = inet_ntoa($host[4]);
  print "$bind_addr\n";
}

my $proxy = IO::Socket::INET->new (
  Proto   => 'tcp',
  LocalPort => $bind_port,
  Type    => SOCK_STREAM,
  Reuse   => 1,
  Listen    => 500
) or die "$!";

binmode $proxy;

logmsg "* Listening on port $bind_port...";

while  (my $client = $proxy->accept()) {

  binmode $client;

  # fork
  die ("Couln't fork!") unless (defined(my $child = fork()));

  if ($child) {

    # fork 1: closing connection to continue accepting new ones
    $client->close();

  } else {

    # fork 2: accepting or rejecting this connection

    my $caddr = inet_ntoa($client->peeraddr);
    my $cport = $client->peerport;

    logmsg "* Client connection from $caddr:$cport...";

    if ($caddr ne $bind_addr) {
      logmsg "* Rejected (expecting connections from: $bind_addr).";
      $client->close();
      exit;
    }

    my $buff;
    my $stage;
    my $step;

    while (my $line = <$client>) {

      # saving headers in a buffer before connecting
      #logdat $line, "[>] ";

      $buff .= $line;

      my ($line, $lbreak) = ($line =~ /^([^\r\n]*)(\r\n|\n)$/sg);

      # headers end when the client sends \r\n (actually, qproxy accepts both \r\n and \n)
      if (!$line) {

        while ($buff =~ /(.+)(\r\n|\n|$)/g) {
          # first header defines the request

          my $headline = $1;

          my ($request, $param, ) = ($headline =~ /(\w+)\s([^\s]+)\s.*/);

          for ($request) {

            # connect method -> CONNECT hostname:port HTTP/X.Y
            if (/CONNECT/i) {

              my ($dest_addr, $dest_port) = ($param =~ /([a-zA-Z0-9\.\-]+):(\d+)/);

              if ($dest_addr && $dest_port) {

                logmsg "* Connecting to $dest_addr:$dest_port...";

                my $server = IO::Socket::INET->new (
                  PeerAddr  => $dest_addr,
                  PeerPort  => $dest_port,
                  Proto   => 'tcp',
                  Type    => SOCK_STREAM
                ) or die "$!";

                binmode $server;

                $server->autoflush(1);

                logmsg "* Connection established.";

                print $client "HTTP/1.0 200 Connection Established\r\n";
                print $client "Proxy-Agent: qproxy.pl\r\n";
                print $client "\r\n";

                my $saddr = inet_ntoa($server->peeraddr);
                my $sport = $server->peerport;

                logmsg "* Forwarding ($saddr:$sport <=> $caddr:$cport)";

                # spawining parallel conversation
                exit ("Couldn't fork!") unless(defined($child = fork()));

                if ($child) {
                  # forwarding server responses to client
                  while (sysread($server, my $sdata, 65535)) {
                    logdat "$sdata", "[$dest_addr:$sport]--> ";
                    syswrite $client, $sdata, length($sdata);
                  }

                  kill("TERM", $child);
                } else {
                  # forwarding client responses to server
                  while (sysread($client, my $cdata, 65535)) {
                    logdat "$cdata", "[$caddr:$cport]--> ";
                    syswrite $server, $cdata, length($cdata);
                  }
                }

                $server->close();

                logmsg "* Server connection closed ($dest_addr:$dest_port)";

              } else {
                logmsg "* Incorrect format of servername:port, at CONNECT request.";
              }
            }
            $buff = "";
          }
          logmsg "* Client connection closed ($caddr:$cport).";
          $client->close();
        }
      }
    }
  }
}
$proxy->close();
