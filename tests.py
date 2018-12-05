#! /usr/bin/python3

import pexpect
import unittest
import re
import os

keytab = {
	"<home>": "\033[1~",
	"<end>": "\033[4~",
	"<ins>": "\033[2~",
	"<del>": "\033[3~",
	"<pgup>": "\033[5~",
	"<pgdown>": "\033[6~",
	"<backspace>": "",
	"<tab>": "\t",
	"<cr>": "\r",
	"<lf>": "\n",
	"<left>": "\033[D",
	"<aleft>": "\033OD",
	"<right>": "\033[C",
	"<aright>": "\033OC",
	"<up>": "\033[A",
	"<aup>": "\033OA",
	"<down>": "\033[B",
	"<adown>": "\033OB",
	"<c-left>": "\033[1;5D",
	"<c-right>": "\033[1;5C",
	"<c-up>": "\033[1;5A",
	"<c-down>": "\033[1;5B",
	"<c-a>": "",
	"<c-b>": "",
	"<c-c>": "",
	"<c-d>": "",
	"<c-e>": "",
	"<c-f>": "",
	"<c-k>": "",
	"<c-l>": "",
	"<c-n>": "",
	"<c-p>": "",
	"<c-r>": "",
	"<c-s>": "",
	"<c-u>": "",
	"<c-v>": "",
	"<c-w>": "",
	"<c-y>": "",
	"<m-b>": "\033b",
	"<m-c>": "\033c",
	"<m-d>": "\033d",
	"<m-f>": "\033f",
	"<m-l>": "\033l",
	"<m-n>": "\033n",
	"<m-p>": "\033p",
	"<m-u>": "\033u",
	"<m-y>": "\033y",
	"<m-backspace>": "\033\177",
	"<f1>": "\033OP",
	"<f2>": "\033OQ"
}

termseq = {
	"\x1bc": "<RIS>",
	"\x1b[0m": "<rst>",
	"\x1b[H": "<mvhm>",
	"\x1b[2J": "<clr>",
	"\x1b[J": "<ceos>",
	"\x1b[0;1;30m": "<gray>",
	"\x1b[0;22;31m": "<red>",
	"\x1b[0;22;32m": "<green>",
	"\x1b[0;1;31m": "<brightred>",
	"\x1b[1;32m": "<brightgreen>",
	"\x1b[0;1;34m": "<brightblue>",
	"\x1b[0;1;35m": "<brightmagenta>",
	"\x1b[0;1;37m": "<white>",
	"\x1b[101;1;33m": "<err>",
	"\x07": "<bell>"
}
colRe = re.compile( "\\x1b\\[(\\d+)G" )
upRe = re.compile( "\\x1b\\[(\\d+)A" )

def sym_to_raw( str_ ):
	for sym, seq in keytab.items():
		str_ = str_.replace( sym, seq )
	return str_

def seq_to_sym( str_ ):
	for seq, sym in termseq.items():
		str_ = str_.replace( seq, sym )
	str_ = colRe.sub( "<c\\1>", str_ )
	str_ = upRe.sub( "<u\\1>", str_ )
	return str_

_words_ = [
	"ada", "algol"
	"bash", "basic",
	"clojure", "cobol", "csharp",
	"eiffel", "erlang",
	"forth", "fortran", "fsharp",
	"go", "groovy",
	"haskell", "huginn",
	"java", "javascript", "julia",
	"kotlin",
	"lisp", "lua",
	"modula",
	"nemerle",
	"ocaml",
	"perl", "php", "prolog", "python",
	"rebol", "ruby", "rust",
	"scala", "scheme", "sql", "swift",
	"typescript"
]

def skip( test_ ):
	return "SKIP" in os.environ and os.environ["SKIP"].find( test_ ) >= 0

class ReplxxTests( unittest.TestCase ):
	_prompt_ = "\033\\[1;32mreplxx\033\\[0m> "
	_cxxSample_ = "./build/example-cxx-api"
	_cSample_ = "./build/example-c-api"
	def check_scenario(
		self_, seq_, expected_,
		history = "one\ntwo\nthree\n",
		term = "xterm",
		command = _cxxSample_,
		dimensions = ( 25, 80 ),
		prompt = _prompt_,
		end = _prompt_ + "\r\nExiting Replxx\r\n",
		encoding = "utf-8"
	):
		with open( "replxx_history.txt", "wb" ) as f:
			f.write( history.encode( encoding ) )
			f.close()
		os.environ["TERM"] = term
		self_._replxx = pexpect.spawn( command, maxread = 1, encoding = encoding, dimensions = dimensions )
		self_._replxx.expect( prompt )
		self_.maxDiff = None
		self_._replxx.send( sym_to_raw( seq_ ) )
		self_._replxx.expect( end )
		self_.assertSequenceEqual( seq_to_sym( self_._replxx.before ), expected_ )
	def test_unicode( self_ ):
		self_.check_scenario(
			"<up><cr><c-d>",
			"<c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><gray><rst><c21>"
			"<c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><c21>\r\n"
			"aóą Ϩ 𓢀  󃔀  \r\n",
			"aóą Ϩ 𓢀  󃔀  \n"
		)
		self_.check_scenario(
			"aóą Ϩ 𓢀  󃔀  <cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>aó<rst><gray><rst><c11><c9><ceos>aóą<rst><gray><rst><c12><c9><ceos>aóą "
			"<rst><gray><rst><c13><c9><ceos>aóą Ϩ<rst><gray><rst><c14><c9><ceos>aóą Ϩ "
			"<rst><gray><rst><c15><c9><ceos>aóą Ϩ 𓢀<rst><gray><rst><c16><c9><ceos>aóą Ϩ 𓢀 "
			"<rst><gray><rst><c17><c9><ceos>aóą Ϩ 𓢀  "
			"<rst><gray><rst><c18><c9><ceos>aóą Ϩ 𓢀  󃔀<rst><gray><rst><c19><c9><ceos>aóą Ϩ 𓢀  󃔀 "
			"<rst><gray><rst><c20><c9><ceos>aóą Ϩ 𓢀  󃔀  "
			"<rst><gray><rst><c21><c9><ceos>aóą Ϩ 𓢀  󃔀  <rst><c21>\r\n"
			"aóą Ϩ 𓢀  󃔀  \r\n"
		)
	@unittest.skipIf( skip( "8bit_encoding" ), "broken platform" )
	def test_8bit_encoding( self_ ):
		LC_CTYPE = "LC_CTYPE"
		exists = LC_CTYPE in os.environ
		lcCtype = None
		if exists:
			lcCtype = os.environ[LC_CTYPE]
		os.environ[LC_CTYPE] = "pl_PL.ISO-8859-2"
		self_.check_scenario(
			"<aup><cr><c-d>",
			"<c9><ceos>text ~ó~<rst><gray><rst><c17><c9><ceos>text ~ó~<rst><c17>\r\ntext ~ó~\r\n",
			"text ~ó~\n",
			encoding = "iso-8859-2"
		)
		if exists:
			os.environ[LC_CTYPE] = lcCtype
		else:
			del os.environ[LC_CTYPE]
	def test_bad_term( self_ ):
		self_.check_scenario(
			"a line of text<cr><c-d>",
			"a line of text\r\na line of text\r\n",
			term = "dumb"
		)
	def test_ctrl_c( self_ ):
		self_.check_scenario(
			"abc<c-c><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c12>^C\r"
			"\r\n"
		)
	def test_ctrl_l( self_ ):
		self_.check_scenario(
			"<cr><cr><cr><c-l><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <RIS><mvhm><clr><rst><brightgreen>replxx<rst>> "
			"<c9><ceos><rst><gray><rst><c9>",
			end = "\r\nExiting Replxx\r\n"
		)
		self_.check_scenario(
			"<cr><up><c-left><c-l><cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos>first "
			"second<rst><gray><rst><c21><c9><ceos>first "
			"second<rst><c15><RIS><mvhm><clr><rst><brightgreen>replxx<rst>> "
			"<c9><ceos>first second<rst><c15><c9><ceos>first second<rst><c21>\r\n"
			"first second\r\n",
			"first second\n"
		)
	def test_backspace( self_ ):
		self_.check_scenario(
			"<up><c-a><m-f><c-right><backspace><backspace><backspace><backspace><cr><c-d>",
			"<c9><ceos>one two three<rst><gray><rst><c22><c9><ceos>one two "
			"three<rst><c9><c9><ceos>one two three<rst><c12><c9><ceos>one two "
			"three<rst><c16><c9><ceos>one tw three<rst><c15><c9><ceos>one t "
			"three<rst><c14><c9><ceos>one  three<rst><c13><c9><ceos>one "
			"three<rst><c12><c9><ceos>one three<rst><c18>\r\n"
			"one three\r\n",
			"one two three\n"
		)
	def test_delete( self_ ):
		self_.check_scenario(
			"<up><m-b><c-left><del><c-d><del><c-d><cr><c-d>",
			"<c9><ceos>one two three<rst><gray><rst><c22><c9><ceos>one two "
			"three<rst><c17><c9><ceos>one two three<rst><c13><c9><ceos>one wo "
			"three<rst><c13><c9><ceos>one o three<rst><c13><c9><ceos>one  "
			"three<rst><c13><c9><ceos>one three<rst><c13><c9><ceos>one three<rst><c18>\r\n"
			"one three\r\n",
			"one two three\n"
		)
	def test_home_key( self_ ):
		self_.check_scenario(
			"abc<home>z<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><c13>\r\n"
			"zabc\r\n"
		)
	def test_end_key( self_ ):
		self_.check_scenario(
			"abc<home>z<end>q<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>zabc<rst><c10><c9><ceos>zabc<rst><gray><rst><c13><c9><ceos>zabcq<rst><gray><rst><c14><c9><ceos>zabcq<rst><c14>\r\n"
			"zabcq\r\n"
		)
	def test_left_key( self_ ):
		self_.check_scenario(
			"abc<left>x<aleft><left>y<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c11><c9><ceos>abxc<rst><c12><c9><ceos>abxc<rst><c11><c9><ceos>abxc<rst><c10><c9><ceos>aybxc<rst><c11><c9><ceos>aybxc<rst><c14>\r\n"
			"aybxc\r\n"
		)
	def test_right_key( self_ ):
		self_.check_scenario(
			"abc<home><right>x<aright>y<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc<rst><c9><c9><ceos>abc<rst><c10><c9><ceos>axbc<rst><c11><c9><ceos>axbc<rst><c12><c9><ceos>axbyc<rst><c13><c9><ceos>axbyc<rst><c14>\r\n"
			"axbyc\r\n"
		)
	def test_prev_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<c-left><c-left>x<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc "
			"<rst><gray><rst><c13><c9><ceos>abc d<rst><gray><rst><c14><c9><ceos>abc "
			"de<rst><gray><rst><c15><c9><ceos>abc def<rst><gray><rst><c16><c9><ceos>abc "
			"def <rst><gray><rst><c17><c9><ceos>abc def "
			"g<rst><gray><rst><c18><c9><ceos>abc def gh<rst><gray><rst><c19><c9><ceos>abc "
			"def ghi<rst><gray><rst><c20><c9><ceos>abc def ghi<rst><c17><c9><ceos>abc def "
			"ghi<rst><c13><c9><ceos>abc xdef ghi<rst><c14><c9><ceos>abc xdef "
			"ghi<rst><c21>\r\n"
			"abc xdef ghi\r\n"
		)
	def test_next_word_key( self_ ):
		self_.check_scenario(
			"abc def ghi<home><c-right><c-right>x<cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>abc<rst><gray><rst><c12><c9><ceos>abc "
			"<rst><gray><rst><c13><c9><ceos>abc d<rst><gray><rst><c14><c9><ceos>abc "
			"de<rst><gray><rst><c15><c9><ceos>abc def<rst><gray><rst><c16><c9><ceos>abc "
			"def <rst><gray><rst><c17><c9><ceos>abc def "
			"g<rst><gray><rst><c18><c9><ceos>abc def gh<rst><gray><rst><c19><c9><ceos>abc "
			"def ghi<rst><gray><rst><c20><c9><ceos>abc def ghi<rst><c9><c9><ceos>abc def "
			"ghi<rst><c12><c9><ceos>abc def ghi<rst><c16><c9><ceos>abc defx "
			"ghi<rst><c17><c9><ceos>abc defx ghi<rst><c21>\r\n"
			"abc defx ghi\r\n"
		)
	def test_hint_show( self_ ):
		self_.check_scenario(
			"co\r<c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>co<rst><c11>\r\n"
			"co\r\n"
		)
	def test_hint_scroll_down( self_ ):
		self_.check_scenario(
			"co<c-down><c-down><tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        "
			"<gray>color_green<rst><u3><c11><c9><ceos>co<rst><gray>lor_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst>\r\n"
			"        "
			"<gray>color_brown<rst><u3><c11><c9><ceos>co<rst><gray>lor_red<rst>\r\n"
			"        <gray>color_green<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        "
			"<gray>color_blue<rst><u3><c11><c9><ceos><red>color_red<rst><green><rst><c18><c9><ceos><red>color_red<rst><c18>\r\n"
			"color_red\r\n"
		)
	def test_hint_scroll_up( self_ ):
		self_.check_scenario(
			"co<c-up><c-up><tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        "
			"<gray>color_green<rst><u3><c11><c9><ceos>co<rst><gray>lor_normal<rst>\r\n"
			"        <gray>co\r\n"
			"        <gray>color_black<rst>\r\n"
			"        "
			"<gray>color_red<rst><u3><c11><c9><ceos>co<rst><gray>lor_white<rst>\r\n"
			"        <gray>color_normal<rst>\r\n"
			"        <gray>co\r\n"
			"        "
			"<gray>color_black<rst><u3><c11><c9><ceos><white>color_white<rst><green><rst><c20><c9><ceos><white>color_white<rst><c20>\r\n"
			"color_white\r\n"
		)
	def test_history( self_ ):
		self_.check_scenario(
			"<up><up><up><up><down><down><down><down>four<cr><c-d>",
			"<c9><ceos>three<rst><gray><rst><c14><c9><ceos>two<rst><gray><rst><c12><c9><ceos>one<rst><gray><rst><c12><c9><ceos>two<rst><gray><rst><c12><c9><ceos>three<rst><gray><rst><c14><c9><ceos><rst><gray><rst><c9><c9><ceos>f<rst><gray><rst><c10><c9><ceos>fo<rst><gray><rst><c11><c9><ceos>fou<rst><gray><rst><c12><c9><ceos>four<rst><gray><rst><c13><c9><ceos>four<rst><c13>\r\n"
			"four\r\n"
		)
		with open( "replxx_history.txt", "rb" ) as f:
			self_.assertSequenceEqual( f.read().decode(), "one\ntwo\nthree\nfour\n" )
	def test_paren_matching( self_ ):
		self_.check_scenario(
			"ab(cd)ef<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>ab<rst><gray><rst><c11><c9><ceos>ab<brightmagenta>(<rst><gray><rst><c12><c9><ceos>ab<brightmagenta>(<rst>c<rst><gray><rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<rst><gray><rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst><gray><rst><c15><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>e<rst><gray><rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><gray><rst><c17><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c16><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c15><c9><ceos>ab<brightred>(<rst>cd<brightmagenta>)<rst>ef<rst><c14><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c13><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c12><c9><ceos>ab<brightmagenta>(<rst>cd<brightred>)<rst>ef<rst><c11><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c10><c9><ceos>ab<brightmagenta>(<rst>cd<brightmagenta>)<rst>ef<rst><c17>\r\n"
			"ab(cd)ef\r\n"
		)
	def test_paren_not_matched( self_ ):
		self_.check_scenario(
			"a(b[c)d<left><left><left><left><left><left><left><cr><c-d>",
			"<c9><ceos>a<rst><gray><rst><c10><c9><ceos>a<brightmagenta>(<rst><gray><rst><c11><c9><ceos>a<brightmagenta>(<rst>b<rst><gray><rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst><gray><rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<rst><gray><rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst><gray><rst><c15><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><gray><rst><c16><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c15><c9><ceos>a<err>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c14><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c13><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c12><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c11><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<err>)<rst>d<rst><c10><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c9><c9><ceos>a<brightmagenta>(<rst>b<brightmagenta>[<rst>c<brightmagenta>)<rst>d<rst><c16>\r\n"
			"a(b[c)d\r\n"
		)
	def test_tab_completion( self_ ):
		self_.check_scenario(
			"co<tab><tab>bri<tab>b<tab><cr><c-d>",
			"<c9><ceos>c<rst><gray><rst><c10><c9><ceos>co<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c11><c9><ceos>color_<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c15><c9><ceos>color_<rst><c15>\r\n"
			"<brightmagenta>color_<rst>black          "
			"<brightmagenta>color_<rst>cyan           "
			"<brightmagenta>color_<rst>brightblue\r\n"
			"<brightmagenta>color_<rst>red            "
			"<brightmagenta>color_<rst>lightgray      "
			"<brightmagenta>color_<rst>brightmagenta\r\n"
			"<brightmagenta>color_<rst>green          "
			"<brightmagenta>color_<rst>gray           "
			"<brightmagenta>color_<rst>brightcyan\r\n"
			"<brightmagenta>color_<rst>brown          "
			"<brightmagenta>color_<rst>brightred      <brightmagenta>color_<rst>white\r\n"
			"<brightmagenta>color_<rst>blue           "
			"<brightmagenta>color_<rst>brightgreen    <brightmagenta>color_<rst>normal\r\n"
			"<brightmagenta>color_<rst>magenta        <brightmagenta>color_<rst>yellow\r\n"
			"<brightgreen>replxx<rst>> <c9><ceos>color_<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_red<rst>\r\n"
			"        <gray>color_green<rst><u3><c15><c9><ceos>color_b<rst><gray><rst>\r\n"
			"        <gray>color_black<rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_blue<rst><u3><c16><c9><ceos>color_br<rst><gray><rst>\r\n"
			"        <gray>color_brown<rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        "
			"<gray>color_brightgreen<rst><u3><c17><c9><ceos>color_bri<rst><gray><rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c18><c9><ceos>color_bright<rst><gray><rst>\r\n"
			"        <gray>color_brightred<rst>\r\n"
			"        <gray>color_brightgreen<rst>\r\n"
			"        "
			"<gray>color_brightblue<rst><u3><c21><c9><ceos>color_brightb<rst><green>lue<rst><c22><c9><ceos><brightblue>color_brightblue<rst><green><rst><c25><c9><ceos><brightblue>color_brightblue<rst><c25>\r\n"
			"color_brightblue\r\n"
		)
	def test_completion_pager( self_ ):
		cmd = ReplxxTests._cSample_ + " q1 x" + ",".join( _words_ )
#		print( cmd );
		self_.check_scenario(
			"<tab>py<cr><c-d>",
			"<c9><ceos><rst><c9>\r\n"
			"<brightmagenta><rst>ada         <brightmagenta><rst>groovy      <brightmagenta><rst>perl\r\n"
			"<brightmagenta><rst>algolbash   <brightmagenta><rst>haskell     <brightmagenta><rst>php\r\n"
			"<brightmagenta><rst>basic       <brightmagenta><rst>huginn      <brightmagenta><rst>prolog\r\n"
			"<brightmagenta><rst>clojure     <brightmagenta><rst>java        <brightmagenta><rst>python\r\n"
			"<brightmagenta><rst>cobol       <brightmagenta><rst>javascript  <brightmagenta><rst>rebol\r\n"
			"<brightmagenta><rst>csharp      <brightmagenta><rst>julia       <brightmagenta><rst>ruby\r\n"
			"<brightmagenta><rst>eiffel      <brightmagenta><rst>kotlin      <brightmagenta><rst>rust\r\n"
			"<brightmagenta><rst>erlang      <brightmagenta><rst>lisp        <brightmagenta><rst>scala\r\n"
			"<brightmagenta><rst>forth       <brightmagenta><rst>lua         <brightmagenta><rst>scheme\r\n"
			"--More--<bell>\r"
			"\t\t\t\t\r"
			"<brightmagenta><rst>fortran     <brightmagenta><rst>modula      <brightmagenta><rst>sql\r\n"
			"<brightmagenta><rst>fsharp      <brightmagenta><rst>nemerle     <brightmagenta><rst>swift\r\n"
			"<brightmagenta><rst>go          <brightmagenta><rst>ocaml       <brightmagenta><rst>typescript\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><gray><rst><c9><c9><ceos><rst><c9>\r\n",
			dimensions = ( 10, 40 ),
			command = cmd
		)
	def test_history_search_backward( self_ ):
		self_.check_scenario(
			"<c-r>repl<c-r><cr><c-d>",
			"<c9><ceos><rst><gray><rst><c9><c1><ceos>(reverse-i-search)`': "
			"<c23><c1><ceos>(reverse-i-search)`r': echo repl "
			"golf<c29><c1><ceos>(reverse-i-search)`re': echo repl "
			"golf<c30><c1><ceos>(reverse-i-search)`rep': echo repl "
			"golf<c31><c1><ceos>(reverse-i-search)`repl': echo repl "
			"golf<c32><c1><ceos>(reverse-i-search)`repl': charlie repl "
			"delta<c35><c1><ceos><brightgreen>replxx<rst>> charlie repl "
			"delta<c17><c9><ceos>charlie repl delta<rst><c27>\r\n"
			"charlie repl delta\r\n",
			"some command\n"
			"alfa repl bravo\n"
			"other request\n"
			"charlie repl delta\n"
			"misc input\n"
			"echo repl golf\n"
			"final thoughts\n"
		)
	def test_history_prefix_search_backward( self_ ):
		self_.check_scenario(
			"repl<m-p><m-p><cr><c-d>",
			"<c9><ceos>r<rst><gray><rst><c10><c9><ceos>re<rst><gray><rst><c11><c9><ceos>rep<rst><gray><rst><c12><c9><ceos>repl<rst><gray><rst><c13><c9><ceos>repl_echo "
			"golf<rst><gray><rst><c23><c9><ceos>repl_charlie "
			"delta<rst><gray><rst><c27><c9><ceos>repl_charlie delta<rst><c27>\r\n"
			"repl_charlie delta\r\n",
			"some command\n"
			"repl_alfa bravo\n"
			"other request\n"
			"repl_charlie delta\n"
			"misc input\n"
			"repl_echo golf\n"
			"final thoughts\n"
		)
	def test_history_browse( self_ ):
		self_.check_scenario(
			"<up><aup><pgup><down><up><up><adown><pgdown><up><down><down><up><cr><c-d>",
			"<c9><ceos>twelve<rst><gray><rst><c15>"
			"<c9><ceos>eleven<rst><gray><rst><c15>"
			"<c9><ceos>one<rst><gray><rst><c12>"
			"<c9><ceos>two<rst><gray><rst><c12>"
			"<c9><ceos>one<rst><gray><rst><c12>"
			"<c9><ceos>two<rst><gray><rst><c12>"
			"<c9><ceos><rst><gray><rst><c9>"
			"<c9><ceos>twelve<rst><gray><rst><c15>"
			"<c9><ceos><rst><gray><rst><c9>"
			"<c9><ceos>twelve<rst><gray><rst><c15>"
			"<c9><ceos>twelve<rst><c15>\r\n"
			"twelve\r\n",
			"one\n"
			"two\n"
			"three\n"
			"four\n"
			"five\n"
			"six\n"
			"seven\n"
			"eight\n"
			"nine\n"
			"ten\n"
			"eleven\n"
			"twelve\n"
		)
	def test_history_max_size( self_ ):
		self_.check_scenario(
			"<pgup><pgdown>a<cr><pgup><cr><c-d>",
			"<c9><ceos>three<rst><gray><rst><c14><c9><ceos><rst><gray><rst><c9><c9><ceos>a<rst><gray><rst><c10><c9><ceos>a<rst><c10>\r\n"
			"a\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos>four<rst><gray><rst><c13><c9><ceos>four<rst><c13>\r\n"
			"four\r\n",
			"one\n"
			"two\n"
			"three\n"
			"four\n"
			"five\n",
			command = ReplxxTests._cSample_ + " q1 s3"
		)
	def test_capitalize( self_ ):
		self_.check_scenario(
			"<up><home><right><m-c><m-c><right><right><m-c><m-c><m-c><cr><c-d>",
			"<c9><ceos>abc defg ijklmn zzxq<rst><gray><rst><c29><c9><ceos>abc defg ijklmn "
			"zzxq<rst><c9><c9><ceos>abc defg ijklmn zzxq<rst><c10><c9><ceos>aBc defg "
			"ijklmn zzxq<rst><c12><c9><ceos>aBc Defg ijklmn zzxq<rst><c17><c9><ceos>aBc "
			"Defg ijklmn zzxq<rst><c18><c9><ceos>aBc Defg ijklmn "
			"zzxq<rst><c19><c9><ceos>aBc Defg iJklmn zzxq<rst><c24><c9><ceos>aBc Defg "
			"iJklmn Zzxq<rst><gray><rst><c29><c9><ceos>aBc Defg iJklmn Zzxq<rst><c29>\r\n"
			"aBc Defg iJklmn Zzxq\r\n",
			"abc defg ijklmn zzxq\n"
		)
	def test_make_upper_case( self_ ):
		self_.check_scenario(
			"<up><home><right><right><right><m-u><m-u><right><m-u><cr><c-d>",
			"<c9><ceos>abcdefg hijklmno pqrstuvw<rst><gray><rst><c34><c9><ceos>abcdefg "
			"hijklmno pqrstuvw<rst><c9><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c10><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c11><c9><ceos>abcdefg hijklmno "
			"pqrstuvw<rst><c12><c9><ceos>abcDEFG hijklmno "
			"pqrstuvw<rst><c16><c9><ceos>abcDEFG HIJKLMNO "
			"pqrstuvw<rst><c25><c9><ceos>abcDEFG HIJKLMNO "
			"pqrstuvw<rst><c26><c9><ceos>abcDEFG HIJKLMNO "
			"PQRSTUVW<rst><gray><rst><c34><c9><ceos>abcDEFG HIJKLMNO "
			"PQRSTUVW<rst><c34>\r\n"
			"abcDEFG HIJKLMNO PQRSTUVW\r\n",
			"abcdefg hijklmno pqrstuvw\n"
		)
	def test_make_lower_case( self_ ):
		self_.check_scenario(
			"<up><home><right><right><right><m-l><m-l><right><m-l><cr><c-d>",
			"<c9><ceos>ABCDEFG HIJKLMNO PQRSTUVW<rst><gray><rst><c34><c9><ceos>ABCDEFG "
			"HIJKLMNO PQRSTUVW<rst><c9><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c10><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c11><c9><ceos>ABCDEFG HIJKLMNO "
			"PQRSTUVW<rst><c12><c9><ceos>ABCdefg HIJKLMNO "
			"PQRSTUVW<rst><c16><c9><ceos>ABCdefg hijklmno "
			"PQRSTUVW<rst><c25><c9><ceos>ABCdefg hijklmno "
			"PQRSTUVW<rst><c26><c9><ceos>ABCdefg hijklmno "
			"pqrstuvw<rst><gray><rst><c34><c9><ceos>ABCdefg hijklmno "
			"pqrstuvw<rst><c34>\r\n"
			"ABCdefg hijklmno pqrstuvw\r\n",
			"ABCDEFG HIJKLMNO PQRSTUVW\n"
		)
	def test_kill_to_beginning_of_line( self_ ):
		self_.check_scenario(
			"<up><home><c-right><c-right><right><c-u><end><c-y><cr><c-d>",
			"<c9><ceos><brightblue>+<rst>abc defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><gray><rst><c32><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c13><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c18><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c19><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>+<rst><gray><rst><c22><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc "
			"defg<brightblue>-<rst><gray><rst><c32><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc defg<brightblue>-<rst><c32>\r\n"
			"-ijklmn zzxq++abc defg-\r\n",
			"+abc defg--ijklmn zzxq+\n"
		)
	def test_kill_to_end_of_line( self_ ):
		self_.check_scenario(
			"<up><home><c-right><c-right><right><c-k><home><c-y><cr><c-d>",
			"<c9><ceos><brightblue>+<rst>abc defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><gray><rst><c32><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c9><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c13><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c18><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>--<rst>ijklmn "
			"zzxq<brightblue>+<rst><c19><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>-<rst><gray><rst><c19><c9><ceos><brightblue>+<rst>abc "
			"defg<brightblue>-<rst><c9><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc "
			"defg<brightblue>-<rst><c22><c9><ceos><brightblue>-<rst>ijklmn "
			"zzxq<brightblue>++<rst>abc defg<brightblue>-<rst><c32>\r\n"
			"-ijklmn zzxq++abc defg-\r\n",
			"+abc defg--ijklmn zzxq+\n"
		)
	def test_kill_next_word( self_ ):
		self_.check_scenario(
			"<up><home><c-right><m-d><c-right><c-y><cr><c-d>",
			"<c9><ceos>alpha charlie bravo delta<rst><gray><rst><c34><c9><ceos>alpha "
			"charlie bravo delta<rst><c9><c9><ceos>alpha charlie bravo "
			"delta<rst><c14><c9><ceos>alpha bravo delta<rst><c14><c9><ceos>alpha bravo "
			"delta<rst><c20><c9><ceos>alpha bravo charlie delta<rst><c28><c9><ceos>alpha "
			"bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"alpha charlie bravo delta\n"
		)
	def test_kill_prev_word_to_white_space( self_ ):
		self_.check_scenario(
			"<up><c-left><c-w><c-left><c-y><cr><c-d>",
			"<c9><ceos>alpha charlie bravo delta<rst><gray><rst><c34><c9><ceos>alpha "
			"charlie bravo delta<rst><c29><c9><ceos>alpha charlie "
			"delta<rst><c23><c9><ceos>alpha charlie delta<rst><c15><c9><ceos>alpha bravo "
			"charlie delta<rst><c21><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"alpha charlie bravo delta\n"
		)
	def test_kill_prev_word( self_ ):
		self_.check_scenario(
			"<up><c-left><m-backspace><c-left><c-y><cr><c-d>",
			"<c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"bravo<brightmagenta>.<rst>delta<rst><gray><rst><c34><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"bravo<brightmagenta>.<rst>delta<rst><c29><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"delta<rst><c23><c9><ceos>alpha<brightmagenta>.<rst>charlie "
			"delta<rst><c15><c9><ceos>alpha<brightmagenta>.<rst>bravo<brightmagenta>.<rst>charlie "
			"delta<rst><c21><c9><ceos>alpha<brightmagenta>.<rst>bravo<brightmagenta>.<rst>charlie "
			"delta<rst><c34>\r\n"
			"alpha.bravo.charlie delta\r\n",
			"alpha.charlie bravo.delta\n"
		)
	def test_kill_ring( self_ ):
		self_.check_scenario(
			"<up><c-w><backspace><c-w><backspace><c-w><backspace><c-u><c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y> <c-y><m-y><m-y><m-y><cr><c-d>",
			"<c9><ceos>delta charlie bravo alpha<rst><gray><rst><c34><c9><ceos>delta "
			"charlie bravo <rst><gray><rst><c29><c9><ceos>delta charlie "
			"bravo<rst><gray><rst><c28><c9><ceos>delta charlie "
			"<rst><gray><rst><c23><c9><ceos>delta "
			"charlie<rst><gray><rst><c22><c9><ceos>delta "
			"<rst><gray><rst><c15>"
			"<c9><ceos>delta<rst><gray><rst><c14>"
			"<c9><ceos><rst><gray><rst><c9>"
			"<c9><ceos>delta<rst><gray><rst><c14>"
			"<c9><ceos>charlie<rst><gray><rst><c16>"
			"<c9><ceos>bravo<rst><gray><rst><c14>"
			"<c9><ceos>alpha<rst><gray><rst><c14>"
			"<c9><ceos>alpha "
			"<rst><gray><rst><c15><c9><ceos>alpha "
			"alpha<rst><gray><rst><c20><c9><ceos>alpha "
			"delta<rst><gray><rst><c20><c9><ceos>alpha "
			"charlie<rst><gray><rst><c22><c9><ceos>alpha "
			"bravo<rst><gray><rst><c20><c9><ceos>alpha bravo "
			"<rst><gray><rst><c21><c9><ceos>alpha bravo "
			"bravo<rst><gray><rst><c26><c9><ceos>alpha bravo "
			"alpha<rst><gray><rst><c26><c9><ceos>alpha bravo "
			"delta<rst><gray><rst><c26><c9><ceos>alpha bravo "
			"charlie<rst><gray><rst><c28><c9><ceos>alpha bravo charlie "
			"<rst><gray><rst><c29><c9><ceos>alpha bravo charlie "
			"charlie<rst><gray><rst><c36><c9><ceos>alpha bravo charlie "
			"bravo<rst><gray><rst><c34><c9><ceos>alpha bravo charlie "
			"alpha<rst><gray><rst><c34><c9><ceos>alpha bravo charlie "
			"delta<rst><gray><rst><c34><c9><ceos>alpha bravo charlie delta<rst><c34>\r\n"
			"alpha bravo charlie delta\r\n",
			"delta charlie bravo alpha\n"
		)
	def test_tab_completion_cutoff( self_ ):
		self_.check_scenario(
			"<tab>n<tab>y<cr><c-d>",
			"<c9><ceos><rst><gray><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><gray><rst><c9><c9><ceos><rst><gray><rst><c9>\r\n"
			"Display all 9 possibilities? (y or n)<ceos>\r\n"
			"<brightmagenta><rst>db            <brightmagenta><rst>hallo         "
			"<brightmagenta><rst>hansekogge    <brightmagenta><rst>quetzalcoatl  "
			"<brightmagenta><rst>power\r\n"
			"<brightmagenta><rst>hello         <brightmagenta><rst>hans          "
			"<brightmagenta><rst>seamann       <brightmagenta><rst>quit\r\n"
			"<brightgreen>replxx<rst>> "
			"<c9><ceos><rst><gray><rst><c9><c9><ceos><rst><c9>\r\n",
			command = ReplxxTests._cSample_ + " q1 c3"
		)
	def test_preload( self_ ):
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>Alice has a cat.<rst><gray><rst><c25>"
			"<c9><ceos>Alice has a cat.<rst><c25>\r\n"
			"Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'pAlice has a cat.'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>Cat  eats  mice.\r\n"
			"<rst><gray><rst><u1><c26><c9><ceos>Cat  eats  mice.\r\n"
			"<rst><u1><c26>\r\n"
			"Cat  eats  mice.\r\n"
			"\r\n",
			command = ReplxxTests._cSample_ + " q1 'pCat\teats\tmice.\r\n'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>M Alice has a cat.<rst><gray><rst><c27>"
			"<c9><ceos>M Alice has a cat.<rst><c27>\r\n"
			"M Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'pMAlice has a cat.'"
		)
		self_.check_scenario(
			"<cr><c-d>",
			"<c9><ceos>M  Alice has a cat.<rst><gray><rst><c28>"
			"<c9><ceos>M  Alice has a cat.<rst><c28>\r\n"
			"M  Alice has a cat.\r\n",
			command = ReplxxTests._cSample_ + " q1 'pM\t\t\t\tAlice has a cat.'"
		)

if __name__ == "__main__":
	unittest.main()

