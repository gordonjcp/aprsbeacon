#! /usr/bin/env python

# the following two variables are used by the target "waf dist"
VERSION='0'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

def options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check(header_name='stdlib.h')
    conf.check(header_name='math.h')

    conf.env.CCFLAGS = ['-O0', '-g3']
    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.6.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package = 'osmgpsmap', uselib_store='OSMGPSMAP', atleast_version = '0.7.3', args = '--cflags --libs')
    conf.check_cfg(package="libgps", uselib_store="GPS", atleast_version = '2.96', args = '--cflags --libs')
    conf.check_cfg(package="glib-2.0", uselib_store="GLIB", atleast_version = '2.28', args = '--cflags --libs')
    conf.check_cfg(package="gio-2.0", uselib_store="GIO", atleast_version = '2.28', args = '--cflags --libs')
def build(bld):
    bld(
        features = 'c cprogram',
        source = ['aprsbeacon.c', 'aprsis.c'],
        target = 'aprsbeacon',
        uselib = "GLIB GIO GPS",
        includes = '. /usr/include')
    # aprsmap
    bld(
        features = 'c cprogram',
        source = ['beaconexplorer.c'],
        target = 'beaconexplorer',
        uselib = "GTK OSMGPSMAP GPS",
        includes = '. /usr/include')
  
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=8
    	

