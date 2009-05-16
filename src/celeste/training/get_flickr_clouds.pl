#!/usr/bin/perl
# Copyright (C) 2008 by Tim Nugent
# timnugent@gmail.com
# 
# This file is part of hugin.
# 
# Hugin is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# Hugin is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Hugin  If not, see <http://www.gnu.org/licenses/>.

# Script to retrieve images of clouds from flickR using by searching tags

use strict;
use warnings;
use Flickr::API;
use Data::Dumper;
use File::stat;
use Digest::MD5 qw(md5 md5_hex md5_base64);

my $max_page = 500;
my $max_photos = 1500;
## Tags, comma separated
my $tags = "clouds";
## Let's not hammer the flickR server too much.. they might get cross
my $sleep_time = 2;
## These need to be filled in:
my $api_key    = "f14cb20df0665e42649e4ddc3e69c1ff";
my $secret = "e83949de627c0fdc";
## Then get the auth token and fill it in too:
my $auth_token = "";
## These can stay blank:
my $frob = "";
my $nsid = "";
## Directories
my $pos_local_dir = "/home/tnugent/gsoc/flickr_clouds/positive_set";
my $neg_local_dir = "/home/tnugent/gsoc/flickr_clouds/negative_set";
my %size = ('Square' => '_s', 'Thumbnail' => '_t', 'Small' => '_m', 'Medium' => '', 'Large' => '_b');

my $api = new Flickr::API({'key'    => $api_key,
                           'secret' => $secret});

&get_token() unless $auth_token;

for my $page (1 .. $max_page){

	print "Retrieving search page $page ...\t";

	my $response = $api->execute_method("flickr.photos.search",{'api_key' => $api_key,
							    	    'auth_token' => $auth_token,
						    	    	    'frob' => $frob,
							    	    'tags' => $tags},
							    	    'per_page'=> 500,
							    	    'page' => $page);

	print "Done.\n";
	#print Dumper $response;
	&download_response($response,$pos_local_dir);
}
exit;

sub get_token{
   
	my $response = $api->execute_method("flickr.auth.getFrob");

	if( ! $response->{success} ) {
		print Dumper $response;
		die "Unable to get frob.\n";
	}else{
		$frob =  $response->{tree}->{children}->[1]->{children}->[0]->{content};
	}

	my $url = $api->request_auth_url('write', $frob);
	system("konqueror \"$url\"");


	$response = $api->execute_method("flickr.auth.getToken",{'api_key' => $api_key,
						    		 'frob' => $frob,
						    		 'method' => "flickr.auth.getToken"});
	if( ! $response->{success} ) {
		print Dumper $response;
		die "Unable to get token.\n";
	}else{
		$auth_token = $response->{tree}->{children}->[1]->{children}->[1]->{children}->[0]->{content};
		$nsid = $response->{tree}->{children}->[1]->{children}->[5]->{attributes}->{nsid};
	}

	print "api_key:\t$api_key\n";
	print "secret:\t\t$secret\n";
	print "frob:\t\t$frob\n";	
	print "auth_token:\t$auth_token\n";
	print "nsid:\t\t$nsid\n";

}

sub download_response {

	my ($response,$dir) = @_;
 	my $total_photos_processed = 0;
   	my $photos = $response->{tree}->{children}[1]->{children};
	
  	for( my $i = 0; $i < $#$photos; $i++ ) {

		$total_photos_processed = 0;
		opendir(DIR, $dir) or die "can't opendir $dir $!";
			while (defined(my $file = readdir(DIR))) {
                		if ($file =~ /prediction/){
					$total_photos_processed++;
                		}
		}
		close DIR;
		
		if( $total_photos_processed < $max_photos ) {
	
    			if( $photos->[$i]->{attributes} ) {
      		
				#my $ph_title = $photos->[$i]->{'attributes'}->{'title'};
     	 			my $server_id = $photos->[$i]->{'attributes'}->{'server'};
      				my $photo_id = $photos->[$i]->{'attributes'}->{'id'};
      				my $photo_secret = $photos->[$i]->{'attributes'}->{'secret'};
				my $photo_farm = $photos->[$i]->{'attributes'}->{'farm'};
      				my $fformat = "jpg";

				## Get available sizes - we want the largest (but not the original)
				my @sizes = ();
				my $get_size = $api->execute_method("flickr.photos.getSizes",{'api_key' => $api_key,
						    		 		      'photo_id' => $photo_id});
			
				if( ! $get_size->{success} ) {
					#print Dumper $get_size;
					print "Unable to get photo $photo_id sizes.\n";
				}else{
					foreach (my $i = 1; $i < scalar @{$get_size->{tree}->{children}->[1]->{children}}; $i += 2){
						my $available = $get_size->{tree}->{children}->[1]->{children}->[$i]->{'attributes'}->{'label'};
						push @sizes,$available unless  $available eq 'Original';				
					}
				}
				next unless scalar @sizes;
				#print Dumper $get_size;
				#print "@sizes"."\n";

				my $pic_url = "http://farm".$photo_farm.".static.flickr.com/".$server_id."/".$photo_id."_".$photo_secret.$size{$sizes[-1]}.".".$fformat;
      				my $fpath = $dir."/".$photo_id."_".$photo_secret.".".${fformat};
			
				#Check if picture exists
        			if( ! -e $fpath ){   
				  
         		 		# Download picture				
					#http://farm{farm-id}.static.flickr.com/{server-id}/{id}_{secret}_[mstb].jpg
          				
	  				print "$pic_url\n";
          				system("wget -q -O \"$fpath\" $pic_url");
					
					if (-e $fpath){
						if (stat($fpath)->size < 3000){
							print $sizes[-1]." not actually available. Deleting and trying $sizes[-2] ...\n";
							
							#my $arguments = 'api_key'.$api_key.'method'.'flickr.photos.getPerms'.'permsread'.'photo_id'.$photo_id;
							#my $api_sig = &create_sign($arguments);							
							#my $get_perms = $api->execute_method("flickr.photos.getPerms",{'api_key' => $api_key,
						    	#	 		                                       'photo_id' => $photo_id,
							#							       'api_sig' => $api_sig,
							#							       'perms' => 'read'});													  							
							#print Dumper $get_perms;
							#exit;
				
							system("rm -f \"$fpath\"");
							$pic_url = "http://farm".$photo_farm.".static.flickr.com/".$server_id."/".$photo_id."_".$photo_secret.$size{$sizes[-2]}.".".$fformat;
      							$fpath = $dir."/".$photo_id."_".$photo_secret.".".${fformat};
	  						print "$pic_url\n";
          						system("wget -q -O \"$fpath\" $pic_url");						


							if (-e $fpath){
								if (stat($fpath)->size < 3000){
									print $sizes[-2]." not actually available. Deleting and giving up.\n";
									system("rm -f \"$fpath\"");
									
								}	
							}							
						}
					}
					sleep $sleep_time;
        			}
			}      			
    		}

  	}
	
} 

sub create_sign{

	my $arguments = shift;
	print "$arguments\n";
	my $signed = $secret.$arguments;
	my $md5 = md5_hex($signed);
	return $md5;

}
