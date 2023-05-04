# vim: set tabstop=4 expandtab textwidht=80 filetype=python:

import sys, logging
logging.basicConfig(level=logging.INFO, stream=sys.stdout)
_log = logging.getLogger(__name__)

import unittest
import time, random

import serial
from serial.threaded import LineReader, ReaderThread

from Chameleon import XModem

PORT = '/dev/ttyACM0'

CONFIG_OPTIONS = 'NONE,MF_ULTRALIGHT,MF_CLASSIC_1K,MF_CLASSIC_1K_7B,MF_CLASSIC_4K,MF_CLASSIC_4K_7B,ISO14443A_SNIFF,ISO14443A_READER'.split(',')
BUTTON_COMMANDS = 'NONE,UID_RANDOM,UID_LEFT_INCREMENT,UID_RIGHT_INCREMENT,UID_LEFT_DECREMENT,UID_RIGHT_DECREMENT,CYCLE_SETTINGS,STORE_MEM,RECALL_MEM,TOGGLE_FIELD,STORE_LOG'.split(',')
LED_OPTIONS = 'NONE,POWERED,TERMINAL_CONN,TERMINAL_RXTX,SETTING_CHANGE,MEMORY_STORED,MEMORY_CHANGED,CODEC_RX,CODEC_TX,FIELD_DETECTED,LOGMEM_FULL'.split(',')
LOGMODE_OPTIONS = 'OFF,MEMORY,LIVE'.split(',')

WAIT_FOR_COMMAND_FINISH = 0.1

class CollectLines(LineReader):

	def connection_made(self, transport):
		super(CollectLines, self).connection_made(transport)
		_log.debug('connection established.')
		self.response=[]

	def write_line(self, line):
		_log.debug('sent line {}'.format(line))
		super(CollectLines, self).write_line(line)
		time.sleep(WAIT_FOR_COMMAND_FINISH)

	def handle_line(self, data):
		_log.debug('line received: {!r}'.format(data))
		self.response += [data]

	def connection_lost(self, exc):
		if exc:
			traceback.print_exc(exc)
		_log.debug('port closed.')

	def clear_response(self):
		self.response=[]

class TestCommands(unittest.TestCase):
	def setUp(self):
		self.serial_connection = serial.Serial(PORT, timeout=5)

		# do not assume LOGMODE to be OFF issues #24, #20
		self.serial_connection.write("LOGMODE=OFF\r".encode("ascii"))
		status = self.serial_connection.readline().decode('ascii').rstrip().split(':')
		if status[0]!='100':
			raise Exception("could not set LOGMODE=OFF on device")

	def tearDown(self):
		if self.serial_connection.is_open:
			self.serial_connection.close()
		self.serial_connection = None

	def testInvalidCommand(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("HELLO")
			result = protocol.response
		self.assertTrue(result[0], "200:UNKOWN COMMAND")


	def testVersion(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("VERSION?")
			result = protocol.response

		self.assertEqual(result, ['101:OK WITH TEXT', 'ChameleonMini RevG 161007 using LUFA 151115 compiled with AVR-GCC 4.9.2', 'Based on the open-source NFC tool ChameleonMini', 'https://github.com/emsec/ChameleonMini', 'commit f46f5b4'])

	def testCharging(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CHARGING?")
			result = protocol.response

		self.assertTrue(result[0] in ['120:FALSE','121:TRUE'])

	def testHelp(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("HELP")
			result = protocol.response

		self.assertEqual(result, ['101:OK WITH TEXT', 'VERSION,CONFIG,UID,READONLY,UPLOAD,DOWNLOAD,RESET,UPGRADE,MEMSIZE,UIDSIZE,RBUTTON,RBUTTON_LONG,LBUTTON,LBUTTON_LONG,LEDGREEN,LEDRED,LOGMODE,LOGMEM,LOGDOWNLOAD,LOGSTORE,LOGCLEAR,SETTING,CLEAR,STORE,RECALL,CHARGING,HELP,RSSI,SYSTICK,SEND_RAW,SEND,GETUID,DUMP_MFU,IDENTIFY,TIMEOUT,THRESHOLD,FIELD'])

	def testRSSI(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RSSI?")
			result = protocol.response

		# ['101:OK WITH TEXT', '    0 mV']
		self.assertTrue(result[0], '101:OK WITH TEXT')
		self.assertRegex(result[1], '\s*(\d|\.|\,)+ mV')

	def testSystick(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("SYSTICK?")
			result = protocol.response

		# ['101:OK WITH TEXT', 'C2DA']
		self.assertTrue(result[0],'101:OK WITH TEXT')
		self.assertTrue(int(result[1],16) <= 65535)

	def testRButtonOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON=?")
			result = protocol.response

		# ['101:OK WITH TEXT', 'NONE,UID_RANDOM,UID_LEFT_INCREMENT,UID_RIGHT_INCREMENT,UID_LEFT_DECREMENT,UID_RIGHT_DECREMENT,CYCLE_SETTINGS,STORE_MEM,RECALL_MEM,TOGGLE_FIELD,STORE_LOG']
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(BUTTON_COMMANDS)])

	def testRButtonGet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in BUTTON_COMMANDS)


	def testRButtonSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("RBUTTON={}".format(random.choice(BUTTON_COMMANDS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("RBUTTON={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')

	def testRButtonLongOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON_LONG=?")
			result = protocol.response

		# ['101:OK WITH TEXT', 'NONE,UID_RANDOM,UID_LEFT_INCREMENT,UID_RIGHT_INCREMENT,UID_LEFT_DECREMENT,UID_RIGHT_DECREMENT,CYCLE_SETTINGS,STORE_MEM,RECALL_MEM,TOGGLE_FIELD,STORE_LOG']
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(BUTTON_COMMANDS)])

	def testRButtonLongGet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON_LONG?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in BUTTON_COMMANDS)


	def testRButtonLongSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("RBUTTON_LONG?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("RBUTTON_LONG={}".format(random.choice(BUTTON_COMMANDS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("RBUTTON_LONG={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')


	def testLButtonOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON=?")
			result = protocol.response

		# ['101:OK WITH TEXT', 'NONE,UID_RANDOM,UID_LEFT_INCREMENT,UID_RIGHT_INCREMENT,UID_LEFT_DECREMENT,UID_RIGHT_DECREMENT,CYCLE_SETTINGS,STORE_MEM,RECALL_MEM,TOGGLE_FIELD,STORE_LOG']
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(BUTTON_COMMANDS)])

	def testLButtonGet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in BUTTON_COMMANDS)


	def testLButtonSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("LBUTTON={}".format(random.choice(BUTTON_COMMANDS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("LBUTTON={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')

	def testLButtonLongOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON_LONG=?")
			result = protocol.response

		# ['101:OK WITH TEXT', 'NONE,UID_RANDOM,UID_LEFT_INCREMENT,UID_RIGHT_INCREMENT,UID_LEFT_DECREMENT,UID_RIGHT_DECREMENT,CYCLE_SETTINGS,STORE_MEM,RECALL_MEM,TOGGLE_FIELD,STORE_LOG']
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(BUTTON_COMMANDS)])

	def testLButtonLongGet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON_LONG?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in BUTTON_COMMANDS)


	def testLButtonLongSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LBUTTON_LONG?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("LBUTTON_LONG={}".format(random.choice(BUTTON_COMMANDS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("LBUTTON_LONG={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')

	def testLEDGreenOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDGREEN=?")
			result = protocol.response

		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(LED_OPTIONS)])

	def testLEDGreen(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDGREEN?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in LED_OPTIONS)

	def testLEDGreenSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDGREEN?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("LEDGREEN={}".format(random.choice(LED_OPTIONS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("LEDGREEN={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')

	def testLEDRedOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDRED=?")
			result = protocol.response

		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(LED_OPTIONS)])

	def testLEDRed(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDRED?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in LED_OPTIONS)

	def testLEDRedSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LEDRED?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("LEDRED={}".format(random.choice(LED_OPTIONS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("LEDRED={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0],'101:OK WITH TEXT')

	def testLogModeOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LOGMODE=?")
			result = protocol.response
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(LOGMODE_OPTIONS)])

	def testLogMode(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LOGMODE?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in LOGMODE_OPTIONS)

	def testLogModeSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LOGMODE?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None
			protocol.write_line("LOGMODE={}".format(random.choice(LOGMODE_OPTIONS)))
			result = protocol.response
			self.assertEqual(result[0],'100:OK')
			protocol.clear_response()

			result = None
			protocol.write_line("LOGMODE={}".format(restore_point))
			result = protocol.response
			protocol.clear_response

			self.assertTrue(result[0], '101:OK WITH TEXT')

	def testLogMem(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LOGMEM?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertRegex(result[1], '[0-9]+')
		self.assertTrue(int(result[1])>=0)
		self.assertTrue(int(result[1])<=2048)

	def testLogClear(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("LOGCLEAR")
			result = protocol.response

		self.assertEqual(result, ['100:OK'])

	def testSetting(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("SETTING?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		setting_slot = int(result[1])
		self.assertTrue(setting_slot>=1 and setting_slot<=8)

	def testSettingSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("SETTING?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			setting_slot = int(result[1])
			self.assertTrue(setting_slot>=1 and setting_slot<=8)
			protocol.clear_response

			for setting_test in range(1,8):
				protocol.write_line("SETTING={}".format(setting_test))
				result = protocol.response
				self.assertEqual(result[0], '101:OK WITH TEXT')
				self.assertTrue(int(result[1]),setting_test)
				protocol.clear_response

			protocol.write_line("SETTING={}".format(setting_slot))
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')

	def testConfigOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG=?")
			result = protocol.response
		self.assertEqual(result, ['101:OK WITH TEXT', ','.join(CONFIG_OPTIONS)])

	def testConfig(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertTrue(result[1] in CONFIG_OPTIONS)

	def testConfigSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG?")
			result = protocol.response

			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = result[1]
			protocol.clear_response()

			result = None

			for config in CONFIG_OPTIONS:
				protocol.write_line("CONFIG={}".format(config))
				result = protocol.response
				self.assertEqual(result[0],'100:OK')
				protocol.clear_response()

			result = None
			protocol.write_line("CONFIG={}".format(restore_point))
			result = protocol.response
			protocol.clear_response()

			self.assertTrue(result[0], '101:OK WITH TEXT')

	def testUIDSize(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("UIDSIZE?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertRegex(result[1], '\d+')
		card_size = int(result[1])
		self.assertTrue(card_size >=0 and card_size <=4096)

	def testUIDConfigNone(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG?")
			current_config = protocol.response[1]
			self.assertTrue(current_config in CONFIG_OPTIONS)
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(CONFIG_OPTIONS[0]))
			#self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("UID?")
			result = protocol.response
			self.assertEqual(result[0], '101:OK WITH TEXT')
			self.assertEqual(result[1], 'NO UID.')

			if not result[1]=='NO UID.':
				self.assertRegex(result[1], '[0-9A-Z]+')
				uid = int(result[1],16)
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(current_config))
			#self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("CONFIG?".format(current_config))
			self.assertEqual(protocol.response[0],'101:OK WITH TEXT')
			self.assertEqual(protocol.response[1],current_config)
			protocol.clear_response()

	def testUIDConfigMF_CLASSIC_1K(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG?")
			current_config = protocol.response[1]
			self.assertTrue(current_config in CONFIG_OPTIONS)
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(CONFIG_OPTIONS[2]))
			#self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("UID?")
			result = protocol.response
			self.assertEqual(result[0], '101:OK WITH TEXT')

			if not result[1]=='NO UID.':
				self.assertRegex(result[1], '[0-9A-Z]+')
				uid = int(result[1],16)
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(current_config))
			#self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("CONFIG?".format(current_config))
			self.assertEqual(protocol.response[0],'101:OK WITH TEXT')
			self.assertEqual(protocol.response[1],current_config)
			protocol.clear_response()

	def testUIDSet(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("CONFIG?")
			current_config = protocol.response[1]
			self.assertTrue(current_config in CONFIG_OPTIONS)
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(CONFIG_OPTIONS[2]))
			#self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("UID?")
			result = protocol.response
			self.assertEqual(result[0], '101:OK WITH TEXT')

			current_uid=None
			if not result[1]=='NO UID.':
				self.assertRegex(result[1], '[0-9A-Z]+')
				current_uid = result[1]
			protocol.clear_response()

			protocol.write_line("UID=12345678")
			self.assertEqual(protocol.response, ['100:OK'])
			protocol.clear_response()

			protocol.write_line("UID=")
			self.assertEqual(protocol.response, ['202:INVALID PARAMETER'])
			protocol.clear_response()

			if current_uid:
				protocol.write_line("UID={}".format(current_uid))
				self.assertEqual(protocol.response, ['100:OK'])
			else:
				protocol.write_line("UID=")
				self.assertEqual(protocol.response, ['202:INVALID PARAMETER'])
			protocol.clear_response()

			protocol.write_line("CONFIG={}".format(current_config))
			self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("CONFIG?".format(current_config))
			self.assertEqual(protocol.response[0],'101:OK WITH TEXT')
			self.assertEqual(protocol.response[1],current_config)
			protocol.clear_response()

	def testReadOnly(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("READONLY?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			self.assertTrue(int(protocol.response[1]) in [0,1])

	def testReadOnlySet(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("READONLY?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			current_state = int(protocol.response[1])
			protocol.clear_response()

			negate_state = int(not current_state)
			protocol.write_line("READONLY={}".format(negate_state))
			self.assertEqual(protocol.response, ['100:OK'])
			protocol.clear_response()

			protocol.write_line("READONLY?")
			self.assertEqual(protocol.response, ['101:OK WITH TEXT', str(negate_state)])
			protocol.clear_response()

			protocol.write_line("READONLY={}".format(current_state))
			self.assertEqual(protocol.response, ['100:OK'])
			protocol.clear_response()

	def testMemSize(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("MEMSIZE?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			self.assertTrue(int(protocol.response[1]) >= 0)
			protocol.clear_response()

	def testDownload(self):
		result = None

		# reader mode
		self.serial_connection.write("CONFIG={}\r".format(CONFIG_OPTIONS[7]).encode("ascii"))
		status = self.serial_connection.readline().decode('ascii').rstrip().split(':')
		_log.debug("rc={}".format(status))

		# initiate download
		self.serial_connection.write("DOWNLOAD\r".encode("ascii"))
		status = self.serial_connection.readline().decode('ascii').rstrip().split(':')
		_log.debug("rc={}".format(status))
		self.assertEqual(status, ['110', 'WAITING FOR XMODEM'])

		modem = XModem(self.serial_connection, _log.debug)
		dump_size = None
		with open("card.dump","wb") as dump_file:
			dump_size = modem.recvData(dump_file)
		self.assertTrue(dump_size>0 and dump_size<=8192)

	def testTimeOutOptions(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("TIMEOUT=?")
			result = protocol.response
		self.assertEqual(result, ['101:OK WITH TEXT', '0 = no timeout', '1-600 = 100 ms - 60000 ms timeout'])

	def testTimeOut(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("TIMEOUT?")
			result = protocol.response

		self.assertEqual(result[0], '101:OK WITH TEXT')
		self.assertRegex(result[1], '\d+\sms')

	def testTimeOutSet(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("TIMEOUT?")
			result = protocol.response
			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = int(result[1].split(" ")[0])//100
			self.assertTrue(restore_point in range(1,600))
			protocol.clear_response()

			protocol.write_line("TIMEOUT={}".format(20))
			self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("TIMEOUT={}".format(restore_point))
			protocol.clear_response()

			self.assertTrue(result[0], '101:OK WITH TEXT')

	def testGetUID(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:

			protocol.write_line("CONFIG={}".format(CONFIG_OPTIONS[7]))
			self.assertEqual(protocol.response,['100:OK'])
			protocol.clear_response()

			protocol.write_line("TIMEOUT?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			time_out = int(protocol.response[1].split(" ")[0])//1000   # in seconds
			protocol.clear_response()

			protocol.write_line("GETUID")

			# if no card is present it will timeout with the message 203:TIMEOUT
			time.sleep(time_out + time_out * 0.1)

			self.assertIn(protocol.response[0],['101:OK WITH TEXT','203:TIMEOUT'])

			timed_out = protocol.response[0] == '203:TIMEOUT'

			if not timed_out:
				print(protocol.response[1])
				self.assertRegex(protocol.response[1], '[0-9A-F]{7,8}')

			protocol.clear_response()

	def testIdentify(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:

			protocol.write_line("CONFIG={}".format(CONFIG_OPTIONS[7]))
			self.assertEqual(protocol.response,['100:OK'])
			protocol.clear_response()

			protocol.write_line("TIMEOUT?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			time_out = int(protocol.response[1].split(" ")[0])//1000   # in seconds
			protocol.clear_response()

			protocol.write_line("IDENTIFY")

			# if no card is present it will timeout with the message 203:TIMEOUT
			time.sleep(time_out + time_out * 0.1)

			self.assertIn(protocol.response[0],['101:OK WITH TEXT','203:TIMEOUT'])

			timed_out = protocol.response[0] == '203:TIMEOUT'

			if not timed_out:
				print(protocol.response[1:])
				self.assertTrue(len(protocol.response)>1)

			protocol.clear_response()

	def testThresholdOptions(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("THRESHOLD=?")
			self.assertEqual(protocol.response,  ['101:OK WITH TEXT', 'Any integer from 0 to 4095. Reference voltage will be (VCC * THRESHOLD / 4095) mV.'])

	def testThreshold(self):
		result = None
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("THRESHOLD?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			self.assertTrue(int(protocol.response[1]) in range(0,4095))

	def testThresholdSet(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("THRESHOLD?")
			result = protocol.response
			self.assertEqual(result[0], '101:OK WITH TEXT')
			restore_point = int(result[1])
			self.assertTrue(restore_point in range(0,4095))
			protocol.clear_response()

			protocol.write_line("THRESHOLD={}".format(300))
			self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

			protocol.write_line("THRESHOLD={}".format(restore_point))
			self.assertEqual(protocol.response[0],'100:OK')
			protocol.clear_response()

	def testField(self):
		with ReaderThread(self.serial_connection, CollectLines) as protocol:
			protocol.write_line("FIELD?")
			self.assertEqual(protocol.response[0], '101:OK WITH TEXT')
			self.assertTrue(int(protocol.response[1]) in range(0,1))


if __name__ == '__main__':
	unittest.main()
