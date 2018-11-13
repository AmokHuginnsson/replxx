#! /usr/bin/python3

import pexpect
import unittest
import os

class ReplxxTests( unittest.TestCase ):
	_prompt_ = "\033\\[1;32mreplxx\033\\[0m> "
	@classmethod
	def setUpClass( cls ):
		os.environ["TERM"] = "xterm"
	def setUp( self_ ):
		self_._replxx = pexpect.spawn( "./build/example-cxx-api", maxread = 1, encoding = "utf-8", dimensions = ( 25, 80 ) )
		self_._replxx.expect( ReplxxTests._prompt_ )
		self_.maxDiff = None
	def check_scenario( self_, seq_, expected_ ):
		self_._replxx.send( seq_ )
		self_._replxx.expect( ReplxxTests._prompt_ + "\r\nExiting Replxx\r\n" )
		self_.assertSequenceEqual( self_._replxx.before, expected_ )
	def test_home_key( self_ ):
		self_.check_scenario(
			"abc\033[1~z\r",
			"\x1b[9G\x1b[Ja\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[10G\x1b[9G\x1b[Jab\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[11G\x1b[9G\x1b[Jabc\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[12G\x1b[9G\x1b[Jabc\x1b[0m\x1b[9G\x1b[9G\x1b[Jzabc\x1b[0m\x1b[10G\x1b[9G\x1b[Jzabc\x1b[0m\x1b[13G\r\n"
			"zabc\r\n"
		)
	def test_end_key( self_ ):
		self_.check_scenario(
			"abc\033[1~z\033[4~q\r",
			"\x1b[9G\x1b[Ja\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[10G\x1b[9G\x1b[Jab\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[11G\x1b[9G\x1b[Jabc\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[12G\x1b[9G\x1b[Jabc\x1b[0m\x1b[9G\x1b[9G\x1b[Jzabc\x1b[0m\x1b[10G\x1b[9G\x1b[Jzabc\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[13G\x1b[9G\x1b[Jzabcq\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[14G\x1b[9G\x1b[Jzabcq\x1b[0m\x1b[14G\r\n"
			"zabcq\r\n"
		)
	def test_hint_show( self_ ):
		self_.check_scenario(
			"co\r",
			"\x1b[9G\x1b[Jc\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[10G\x1b[9G\x1b[Jco\x1b[0m\x1b[0;1;30m\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_black\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_red\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_green\x1b[0m\x1b[3A\x1b[11G\x1b[9G\x1b[Jco\x1b[0m\x1b[11G\r\n"
			"co\r\n"
		)
	def test_hint_scroll_down( self_ ):
		self_.check_scenario(
			"co\033[1;5B\033[1;5B\t\r",
			"\x1b[9G\x1b[Jc\x1b[0m\x1b[0;1;30m\x1b[0m\x1b[10G\x1b[9G\x1b[Jco\x1b[0m\x1b[0;1;30m\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_black\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_red\x1b[0m\r\n"
			"        "
			"\x1b[0;1;30mcolor_green\x1b[0m\x1b[3A\x1b[11G\x1b[9G\x1b[Jco\x1b[0m\x1b[0;1;30mlor_black\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_red\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_green\x1b[0m\r\n"
			"        "
			"\x1b[0;1;30mcolor_brown\x1b[0m\x1b[3A\x1b[11G\x1b[9G\x1b[Jco\x1b[0m\x1b[0;1;30mlor_red\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_green\x1b[0m\r\n"
			"        \x1b[0;1;30mcolor_brown\x1b[0m\r\n"
			"        "
			"\x1b[0;1;30mcolor_blue\x1b[0m\x1b[3A\x1b[11G\x1b[9G\x1b[J\x1b[0;22;31mcolor_red\x1b[0m\x1b[0;22;32m\x1b[0m\x1b[18G\x1b[9G\x1b[J\x1b[0;22;31mcolor_red\x1b[0m\x1b[18G\r\n"
			"color_red\r\n"
		)

if __name__ == '__main__':
	unittest.main()

