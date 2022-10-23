// Copyright 2016 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/system.h"

#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>  // NOLINT(build/include_alpha)
#include <netdb.h>
#include <sys/ioctl.h>
#include "starboard/common/log.h"
#include "starboard/common/string.h"
#include  <linux/sockios.h>
#include  <linux/ethtool.h>

int set_network_status(const char * devicename)
{
  struct ifreq ifr;
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    return -1;
  }
  SbStringCopy(ifr.ifr_name, devicename,SbStringGetLength(devicename)+1);
  struct ethtool_value edata;
  edata.cmd = ETHTOOL_GLINK;
  edata.data = 0;
  ifr.ifr_data = (char *) &edata;
  if(ioctl( sockfd, SIOCETHTOOL, &ifr ) == -1)
  {
    close(sockfd);
    return -1;
  }
  if (edata.data == 1)
  {
    return 0;
  }
  else
  {
    return 1;
  }
  return 1;
}

SbSystemConnectionType SbSystemGetConnectionType() {
  const char * devicename = "eth0";
  if (set_network_status(devicename) == 0)
    return kSbSystemConnectionTypeWired;
  else
    return kSbSystemConnectionTypeWireless;
}
