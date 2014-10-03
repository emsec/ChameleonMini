#!/usr/bin/env python
# -*- coding: latin-1 -*-
#
# Written by Chema Garcia
#    @sch3m4
#    chema@safetybits.net || http://safetybits.net
#

import os
import time
import sys
import argparse
import serial.tools.list_ports
from colorama import init as coloramaInit
from colorama import Fore, Back

from lib import pyCham

cham = pyCham()

def showBanner(showFooter = False):
	print "██████╗ ██╗   ██╗ ██████╗██╗  ██╗ █████╗ ███╗   ███╗"
	print "██╔══██╗╚██╗ ██╔╝██╔════╝██║  ██║██╔══██╗████╗ ████║ by Chema Garcia"
	print "██████╔╝ ╚████╔╝ ██║     ███████║███████║██╔████╔██║    @sch3m4"
	print "██╔═══╝   ╚██╔╝  ██║     ██╔══██║██╔══██║██║╚██╔╝██║    chema@safetybits.net"
	print "██║        ██║   ╚██████╗██║  ██║██║  ██║██║ ╚═╝ ██║ https://github.com/sch3m4/pycham"
	print "╚═╝        ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝ v0.2b"
	if showFooter is True:
		print "a Python interface to Chameleon Mini"
	print ""


def locateDevice ( devid ):
	'''
	Returns the serial port path of the arduino if found, or None if it isn't connected
	'''
	retval = None
	for port in serial.tools.list_ports.comports():
		if len(port[2]) >= len(devid) and port[2][:len(devid)] == devid:
			retval = port[0]
			break

	return retval


def auxMenu(options):
	os.system('clear')
	showBanner()

	ret = None
	while True:
		cont = 1
		for opt in options:
			print Fore.RED + "%s" % cont + Fore.RESET + ")\t" + "%s" % opt
			cont += 1
		print "0) Exit"
	
		try:
			print ""
			opt = int(raw_input ( "Value: " ).strip())
		except:
			continue

		if opt > len ( options ):
			continue

		if int(opt) == 0:
			ret = None
		else:
			ret = options[opt-1]
		break

	return ret


def showMenu():
	cmds = cham.getCommands()
	instantcmds = cham.getInstantCommands()
	map = {}
	finish = False
	while finish is False:
		print "Getting device version..."
		version = cham.getVersion()
		print "Getting available configurations..."
		configurations = cham.getConfigurations()
		print "Getting the currently activated setting..."
		settings = cham.getCurSettings()	
		print "Getting current config..."
		config = cham.getCurConfig()
		print "Getting button actions..."
		bactions = cham.getButtonActions()
		print "Getting all available button actions..."
		bcuractions = cham.getCurButtonActions()
		print "Getting readonly status..."
		rostatus = cham.getReadOnly()
		print "Getting current UID..."
		curuid = cham.getCurrentUID()
		print "Getting current UID size..."
		curuidsize = cham.getCurrentUIDSize()
		print "Getting current memory size..."
		curmemsize = cham.getCurrentMemSize()

		os.system('clear')
		showBanner()
		print "-==== Global Configurations ====-"
		print Fore.RESET + " = " + Fore.MAGENTA + "+ Version:                " + Fore.GREEN + "%s" % version
		print Fore.RESET + " = " + Fore.MAGENTA + "+ Configs:                " + Fore.GREEN + "%s" % configurations
		print Fore.RESET + " = " + Fore.MAGENTA + "+ Button actions:         " + Fore.GREEN + "%s" % bactions
		print Fore.RESET + " = " + Fore.MAGENTA + "+ ReadOnly status:        " + Fore.GREEN + "%s" % rostatus
		print Fore.RESET + " = " + Fore.YELLOW + "+ Current button action:  " + Fore.GREEN + "%s" % bcuractions
		print Fore.RESET + " = " + Fore.YELLOW + "+ Currrent config:        " + Fore.GREEN + "%s" % config
		print Fore.RESET + " = " + Fore.YELLOW + "+ Current setting:        " + Fore.GREEN + "%s" % settings
		print Fore.RESET + " = " + Fore.YELLOW + "+ Current UID:            " + Fore.GREEN + "%s" % curuid
		print Fore.RESET + " = " + Fore.YELLOW + "+ Current UID size:       " + Fore.GREEN + "%s" % curuidsize
		print Fore.RESET + " = " + Fore.YELLOW + "+ Current memory size:    " + Fore.GREEN + "%s" % curmemsize
		print Fore.RESET + "-===============================-\n"

		options = cmds.keys()
		cont = 1
		for opt in options:
			print Fore.RED + "%s" % cont + Fore.RESET + ")\t%s" % cmds[opt]
			cont += 1
			if len(map.keys()) != len(options):
				map[cont] = opt

		print "0)\tExit"			

		correct = False
		reload = False
		while correct is False and reload is False:
			try:
				print ""
				print Fore.BLUE + "NOTE:" + Fore.RESET + " Press enter to reload the menu"
				sel = raw_input("Action: ")
				sel = int(sel)
			except ValueError:
				if len(sel.strip()) == 0:
					reload = True
			except:
				continue

			if reload is True or sel <= len ( options ):
				correct = True
		
		if reload is True:
			continue

		if sel == 0:
			return

		invalid = False
		unknown = False
		cmd = map[sel+1]
		if cmd in instantcmds:
			cham.execute ( cmd )
		elif cmd == pyCham.COMMAND_SET_UID:
			uid = ''
			while len ( uid ) == 0:
				uid = raw_input ( "Enter the new UID value: " ).strip()
			if uid is None:
				unknown = True
			else:
				cmd += uid
		elif cmd == pyCham.COMMAND_SET_CONFIG:
			print "TODO"
		elif cmd == pyCham.COMMAND_SET_RO:
			if int(rostatus) == 0:
				cmd += '1'
			elif int(rostatus) == 1:
				cmd += '0'
			else:
				invalid = True
		elif cmd == pyCham.COMMAND_SET_BUTTON:
			opt = auxMenu ( bactions.split(',') )
			if opt is None:
				unknown = True
			else:
				cmd += opt
		elif cmd == pyCham.COMMAND_SET_SETTING:
			opt = auxMenu ( ['1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16'] )
			if opt is None:
				unknown = True
			else:
				cmd += opt
		elif cmd == pyCham.COMMAND_DOWNLOAD:
			path = '.'
			while os.path.isdir(path) is True and os.path.exists(path):
				path = raw_input ( "Enter destination file path: " ).strip()
			cham.execute ( cmd )
			time.sleep(1)
			cham.downloadToFile ( path , int(curmemsize) )
			print "Restarting..."
#			finish = True
		elif cmd == pyCham.COMMAND_UPLOAD:
			path = ''
			while os.path.isfile(path) is False:
				path = raw_input ( "Enter source file path: " ).strip()
			cham.execute ( cmd )
			time.sleep(1)
			cham.uploadFromFile ( path )
#			finish = True
			print "Restarting..."
		else:
			unknown = True
			print "Unknown option!"

		# this should not happen
		if invalid is True:
			print "Invalid value!"
		elif unknown is False and finish is False:
			cham.execute ( cmd )

		raw_input("Press ENTER to continue")

	return finish


def main():
	showBanner(True)

	if len(sys.argv) < 2:
		print "[i] Trying to autodetect Chameleon-Mini"
		time.sleep(0.5)
		args = {'serial': locateDevice ( pyCham.DEVICE_ID ) }
		if args['serial'] is not None:
			print "[i] Chameleon-Mini found at " + Fore.GREEN + "%s" % args['serial'] + Fore.RESET
		time.sleep(1)
	else:
		parser = argparse.ArgumentParser()
		parser.add_argument('serial', metavar='tty', type=str , help='Serial port of Chameleon' )
		args = vars(parser.parse_args())

	if args['serial'] is None or not os.path.exists ( args['serial' ] ):
		print "[e] Error: Chameleon serial port not found. Rerun this script with -h parameter to view help."
		sys.exit(-1)

	coloramaInit()

	finish = False
	while finish is False:
		cham.setSerial ( args['serial'] )
		if cham.openSerial() is None:
			print "[e] Error opening serial port"
			sys.exit(-2)

		finish = showMenu()
		cham.close()

	cham.close()
	return finish


if __name__ == "__main__":
	finish = False
	while finish is False:
		try:
			finish = main()
		except serial.SerialException:
			print "[e] Connection closed"
			for i in range(0,pyCham.RECON_DELAY):
				val = pyCham.RECON_DELAY - i
				print '[+] Reconnection in %d seconds\r' % val,
				sys.stdout.flush()
				time.sleep(1)
