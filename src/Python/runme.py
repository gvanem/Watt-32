#!/usr/bin/env python

from __future__ import print_function

import sys, inspect, argparse
from win32api import Sleep

import watt32 as w32

class Colour():
  WHITE = YELLOW = RESET = ""

try:
  from colorama import init, Fore, Style
  init()
  Colour.WHITE  = Style.BRIGHT + Fore.WHITE
  Colour.YELLOW = Style.BRIGHT + Fore.YELLOW
  Colour.RESET  = Style.RESET_ALL
except:
  pass

opt = None

def cprint (colour, s):
  print ("%s%s%s" % (colour, s, Colour.RESET))

def __LINE__():
  try:
    raise Exception
  except:
    return sys.exc_info()[2].tb_frame.f_back.f_lineno

def __FILE__():
  return inspect.currentframe().f_code.co_filename

def ping (host):
  idx = 100
  ip  = w32.lookup_host (host, None)
  num = 0
  if ip == 0:
     err = w32.cvar.dom_errno
     cprint (Colour.YELLOW, "Unknown host %s, dom_errno: %d: %s" % (host, err, dom_strerror(err)))
     return

  while num < 5:
    try:
      if w32._ping (ip, idx, None, 0):
         print ("sent PING # %lu " % idx)
    except TypeError as e:
       print ("except: %s " % e)
    idx += 1
    num += 1
    w32.tcp_tick (None)
    if w32._chk_ping (ip, None) != -1:
       cprint (Colour.YELLOW, "Got ICMP echo")
    Sleep (1000)

#
# Show the contents of object 'x'
#
def show_content (opt, x, name):
  cprint (Colour.YELLOW, "%s contains:" % name)
  for _x in sorted(dir(x)):
      if opt.version < 2 and _x.startswith("_swig"):
         continue
      cprint (Colour.WHITE, "  " + _x)
  print (Colour.RESET)

def show_version (opt):
  if opt.version >= 2:
     show_content (opt, w32, repr(w32.__file__))

  if opt.version >= 3:
     show_content (opt, w32._SwigNonDynamicMeta, "w32._SwigNonDynamicMeta")

  if opt.version >= 2:
     if getattr(w32, "sockaddr_in", None):
        cprint (Colour.WHITE, "Built with '-DUSE_SOCKET_API'")
        show_content (opt, w32.sockaddr_in, "w32.sockaddr_in")
     else:
        cprint (Colour.WHITE, "Not built with '-DUSE_SOCKET_API'\n")

     print ("Is GIL-enabled: ", end="")
     if getattr(sys, "_is_gil_enabled", None):
        print (sys._is_gil_enabled())
     else:
        print ("<N/A>")

  print ("Version:   %s"        % w32.wattcpVersion())
  print ("$(CC):     %s (-D%s)" % (w32.wattcpBuildCCexe(), w32.wattcpBuildCC()))
  print ("$(CFLAGS): %s"        % w32.wattcpBuildCflags())

  features = w32.wattcpCapabilities().split ("/")
  length = 0
  print ("Features:  ", end="")
  for f in features[1:]:
      print (f + " ", end="")
      length += len(f)
      if length > 60:
         length = 0
         print ("\n", " "*10, end="")
  print ("")
  sys.exit (0)

def show_help():
  print ("""Usage: %s [-d] [-v] [-h] <host to ping>
    -d:  sets debug level 1.
    -dd: sets debug level 2 etc.
    -v:  show simple version info and exit.
    -vv: show detailed version info and exit.""" % sys.argv[0])
  sys.exit (0)

def parse_cmdline():
  if len(sys.argv) < 2:
     show_help()
  print ("%s (%u): sys.argv[1]: %s" % (__FILE__(), __LINE__(), sys.argv[1]))

  parser = argparse.ArgumentParser (add_help = False)
  parser.add_argument ("-d", dest = "debug",   action="count", default=0)
  parser.add_argument ("-v", dest = "version", action="count")
  parser.add_argument ("-h", dest = "help",    action="store_true")
  parser.add_argument ("host_to_ping", nargs = "?", default = None )
  opt = parser.parse_args()
  if opt.help:
     show_help()

  if opt.version:
     show_version (opt)
  if not opt.host_to_ping:
     opt.host_to_ping = "www.google.com"

  return opt

#
# Our main() function
#
def main():
  opt = parse_cmdline()
  print ("%s (%u): opt.debug %d:" % (__FILE__(), __LINE__(), opt.debug))

  if opt.debug > 0:
     w32.cvar.debug_on = opt.debug
     w32.dbug_init()
  w32.sock_init()
  ping (opt.host_to_ping)

if __name__ == "__main__":
  main()
