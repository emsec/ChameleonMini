#!/usr/bin/env python
#
# Written by Chema Garcia
#    @sch3m4
#    chema@safetybits.net || http://safetybits.net
#

import serial
import time
from xmodem import XMODEM
from StringIO import StringIO

class pyCham():
	# reconnection delay
	RECON_DELAY = 7
	# Chameleon-Mini ID
	DEVICE_ID = 'USB VID:PID=03eb:2044 SNR=435503430313FFFF91FF71000100'
	# commands config
	COMMAND_GET_VERSION = 'version?'
	COMMAND_GET_CONFIGS = 'config'
	COMMAND_GET_CURRENT_CONFIG = 'config?'
	COMMAND_GET_CURRENT_SETTING = 'setting?'
	COMMAND_GET_BUTTON = 'button'
	COMMAND_GET_CURRENT_BUTTON = 'button?'
	COMMAND_GET_CURRENT_UID = 'uid?'
	COMMAND_GET_CURRENT_UID_SIZE = 'uidsize?'
	COMMAND_GET_CURRENT_MEMSIZE = 'memsize?'
	COMMAND_GET_RO = 'readonly?'
	COMMAND_SET_UID = 'uid='
	COMMAND_SET_CONFIG = 'config='
	COMMAND_SET_RO = 'readonly='
	COMMAND_SET_BUTTON = 'button='
	COMMAND_SET_SETTING = 'setting='
	COMMAND_UPLOAD = 'upload'
	COMMAND_DOWNLOAD = 'download'
	COMMAND_RESET = 'reset'
	#COMMAND_UPGRADE = 'upgrade' # not enabled
	COMMAND_CLEAR = 'clear'
	
	INSTANT_COMMANDS = [COMMAND_GET_VERSION,COMMAND_GET_CONFIGS,COMMAND_GET_CURRENT_CONFIG,COMMAND_GET_CURRENT_UID_SIZE, COMMAND_GET_CURRENT_UID , COMMAND_GET_RO,COMMAND_GET_CURRENT_MEMSIZE,COMMAND_GET_BUTTON,COMMAND_GET_CURRENT_BUTTON,COMMAND_GET_CURRENT_SETTING,COMMAND_CLEAR,COMMAND_RESET]
	COMMANDS = {	COMMAND_RESET: 'Resets the Chameleon',
			COMMAND_SET_CONFIG: 'Sets the active configuration',
			COMMAND_SET_UID: 'Sets a new UID (hex)',
			COMMAND_SET_RO: 'Switches the state of the read-only mode',
			COMMAND_UPLOAD: 'Uploads a memory dump upto the memory size',
			COMMAND_DOWNLOAD: 'Downloads a memory dump',
			COMMAND_SET_BUTTON: 'Sets the current button press action for BUTTON0',
			COMMAND_SET_SETTING: 'Sets the active setting',
			COMMAND_CLEAR: 'Clears the entire memory of the currently activated setting' }

	# response commands
	RESPONSE_OK_TEXT = 101
	RESPONSE_OK = 0
	RESPONSE_WAITING = 1
	RESPONSE_ERROR = 2
	RESPONSES = {RESPONSE_OK : [100,RESPONSE_OK_TEXT] , RESPONSE_WAITING : [110] , RESPONSE_ERROR : [200,201,202] }

	# serial port configuration
	SERIAL_TO = 15
	SERIAL_BD = 38400

	def __init__(self):
		self.SERIAL = None
		self.SERIAL_PORT = None

	def setSerial(self,port):
		self.SERIAL_PORT = port

	def openSerial(self):
		if self.SERIAL_PORT is None:
			return None
		try:
			self.SERIAL = serial.Serial(self.SERIAL_PORT,baudrate=self.SERIAL_BD,timeout=self.SERIAL_TO)
		except Exception,e:
			print "Error: Cannot open serial port: %s" % e
			return None
		return self.SERIAL

	def getCommands(self):
		return self.COMMANDS

	def getInstantCommands(self):
		return self.INSTANT_COMMANDS

	def close(self):
		try:
			self.SERIAL.close()
		except:
			pass

	def __check_retcode__ ( self , retcode ):
		"""
		Checks the return code and returns one of the following return codes: RESPONSE_OK | RESPONSE_WAITING | RESPONSE_ERROR
		"""
		code = retcode.split(':')[0]
		ret = None
		for r in self.RESPONSES:
			if int(code) in self.RESPONSES[r]:
				ret = r
				break
		return ret

	def execute(self,command='version?'):
		"""
		Executes an action and returns the pair (return_code,return_value)
		"""
		self.SERIAL.write ( command + '\r' )
		retval = ''
		retcode = self.SERIAL.readline()
		if int(retcode.split(':')[0]) == self.RESPONSE_OK_TEXT:
			retval = self.SERIAL.readline()
		return (retcode.strip(),retval.strip())

	def downloadToFile ( self , tofile='downloaded.bin' , memsize = 1024 ):
		def __read_byte ( size , timeout = 1 ):
			return self.SERIAL.read(size)

		def __write_byte ( data , timeout = 1 ):
			return self.SERIAL.write(data)

		time.sleep(0.1)
		buffer = StringIO()
		stream = open(tofile, 'wb')
		modem = XMODEM(__read_byte,__write_byte).recv(buffer,crc_mode=0,quiet=1)
		contents = buffer.getvalue()
		stream.write(contents[:memsize])
		stream.close()
		buffer.close()
		try:
			self.execute ( self.COMMAND_RESET )
		except:
			pass
		return

	def uploadFromFile ( self , fromfile = 'upload.bin' ):
		def __read_byte ( size , timeout = 1 ):
			return self.SERIAL.read(size)

		def __write_byte ( data , timeout = 1 ):
			return self.SERIAL.write(data)

		time.sleep(0.1)
		stream = open(fromfile, 'rb')
		buffer = StringIO(stream.read())
		stream.close()
		modem = XMODEM(__read_byte,__write_byte).send(buffer,quiet=1)
		buffer.close()
		try:
			self.execute ( self.COMMAND_RESET )
		except:
			pass
		return

	def getVersion(self):
		"""
		Returns the version of Chameleon
		"""
		code,val = self.execute ( self.COMMAND_GET_VERSION )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getConfigurations(self):
		"""
		Returns all profiles available
		"""
		code,val = self.execute ( self.COMMAND_GET_CONFIGS )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getCurSettings(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_SETTING )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getCurConfig(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_CONFIG )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getButtonActions(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_BUTTON )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getCurButtonActions(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_BUTTON )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getReadOnly(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_RO )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None


	def getCurrentUID(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_UID )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getCurrentUIDSize(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_UID_SIZE )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

	def getCurrentMemSize(self):
		"""
		Returns the current settings
		"""
		code,val = self.execute ( self.COMMAND_GET_CURRENT_MEMSIZE )
		retcode = self.__check_retcode__ ( code )
		if retcode == self.RESPONSE_WAITING: # volver a leer
			print "TODO"
		elif retcode == self.RESPONSE_ERROR: # raise an error
			print "TODO"
		elif retcode == self.RESPONSE_OK:
			return val
		else:
			print "Unknown Error"
		return None

