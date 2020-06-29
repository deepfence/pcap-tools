module RDP;

export {
	# http://www.c-amie.co.uk/technical/mstsc-versions/
	const builds = {
		[0419] = "RDP 4.0",
		[2195] = "RDP 5.0",
		[2221] = "RDP 5.0",
		[2600] = "RDP 5.1",
		[3790] = "RDP 5.2",
		[6000] = "RDP 6.0",
		[6001] = "RDP 6.1",
		[6002] = "RDP 6.2",
		[7600] = "RDP 7.0",
		[7601] = "RDP 7.1",
		[9200] = "RDP 8.0",
		[9600] = "RDP 8.1",
		[25189] = "RDP 8.0 (Mac)",
		[25282] = "RDP 8.0 (Mac)"
	} &default = function(n: count): string { return fmt("client_build-%d", n); };

	const security_protocols = {
		[0x00] = "RDP",
		[0x01] = "SSL",
		[0x02] = "HYBRID",
		[0x08] = "HYBRID_EX"
	} &default = function(n: count): string { return fmt("security_protocol-%d", n); };

	const failure_codes = {
		[0x01] = "SSL_REQUIRED_BY_SERVER",
		[0x02] = "SSL_NOT_ALLOWED_BY_SERVER",
		[0x03] = "SSL_CERT_NOT_ON_SERVER",
		[0x04] = "INCONSISTENT_FLAGS",
		[0x05] = "HYBRID_REQUIRED_BY_SERVER",
		[0x06] = "SSL_WITH_USER_AUTH_REQUIRED_BY_SERVER"
	} &default = function(n: count): string { return fmt("failure_code-%d", n); };

	const cert_types = {
		[1] = "RSA",
		[2] = "X.509"
	} &default = function(n: count): string { return fmt("cert_type-%d", n); };

	const encryption_methods = {
		[0] = "None",
		[1] = "40bit",
		[2] = "128bit",
		[8] = "56bit",
		[10] = "FIPS"
	} &default = function(n: count): string { return fmt("encryption_method-%d", n); };

	const encryption_levels = {
		[0] = "None",
		[1] = "Low",
		[2] = "Client compatible",
		[3] = "High",
		[4] = "FIPS"
	} &default = function(n: count): string { return fmt("encryption_level-%d", n); };

	const high_color_depths = {
		[0x0004] = "4bit",
		[0x0008] = "8bit",
		[0x000F] = "15bit",
		[0x0010] = "16bit",
		[0x0018] = "24bit"
	} &default = function(n: count): string { return fmt("high_color_depth-%d", n); };

	const color_depths = {
		[0x0001] = "24bit",
		[0x0002] = "16bit",
		[0x0004] = "15bit",
		[0x0008] = "32bit"
	} &default = function(n: count): string { return fmt("color_depth-%d", n); };

	const results = {
		[0] = "Success",
		[1] = "User rejected",
		[2] = "Resources not available",
		[3] = "Rejected for symmetry breaking",
		[4] = "Locked conference",
	} &default = function(n: count): string { return fmt("result-%d", n); };

	# http://msdn.microsoft.com/en-us/goglobal/bb964664.aspx
	const languages = {
		[1078] = "Afrikaans - South Africa",
		[1052] = "Albanian - Albania",
		[1156] = "Alsatian",
		[1118] = "Amharic - Ethiopia",
		[1025] = "Arabic - Saudi Arabia",
		[5121] = "Arabic - Algeria",
		[15361] = "Arabic - Bahrain",
		[3073] = "Arabic - Egypt",
		[2049] = "Arabic - Iraq",
		[11265] = "Arabic - Jordan",
		[13313] = "Arabic - Kuwait",
		[12289] = "Arabic - Lebanon",
		[4097] = "Arabic - Libya",
		[6145] = "Arabic - Morocco",
		[8193] = "Arabic - Oman",
		[16385] = "Arabic - Qatar",
		[10241] = "Arabic - Syria",
		[7169] = "Arabic - Tunisia",
		[14337] = "Arabic - U.A.E.",
		[9217] = "Arabic - Yemen",
		[1067] = "Armenian - Armenia",
		[1101] = "Assamese",
		[2092] = "Azeri (Cyrillic)",
		[1068] = "Azeri (Latin)",
		[1133] = "Bashkir",
		[1069] = "Basque",
		[1059] = "Belarusian",
		[1093] = "Bengali (India)",
		[2117] = "Bengali (Bangladesh)",
		[5146] = "Bosnian (Bosnia/Herzegovina)",
		[1150] = "Breton",
		[1026] = "Bulgarian",
		[1109] = "Burmese",
		[1027] = "Catalan",
		[1116] = "Cherokee - United States",
		[2052] = "Chinese - People's Republic of China",
		[4100] = "Chinese - Singapore",
		[1028] = "Chinese - Taiwan",
		[3076] = "Chinese - Hong Kong SAR",
		[5124] = "Chinese - Macao SAR",
		[1155] = "Corsican",
		[1050] = "Croatian",
		[4122] = "Croatian (Bosnia/Herzegovina)",
		[1029] = "Czech",
		[1030] = "Danish",
		[1164] = "Dari",
		[1125] = "Divehi",
		[1043] = "Dutch - Netherlands",
		[2067] = "Dutch - Belgium",
		[1126] = "Edo",
		[1033] = "English - United States",
		[2057] = "English - United Kingdom",
		[3081] = "English - Australia",
		[10249] = "English - Belize",
		[4105] = "English - Canada",
		[9225] = "English - Caribbean",
		[15369] = "English - Hong Kong SAR",
		[16393] = "English - India",
		[14345] = "English - Indonesia",
		[6153] = "English - Ireland",
		[8201] = "English - Jamaica",
		[17417] = "English - Malaysia",
		[5129] = "English - New Zealand",
		[13321] = "English - Philippines",
		[18441] = "English - Singapore",
		[7177] = "English - South Africa",
		[11273] = "English - Trinidad",
		[12297] = "English - Zimbabwe",
		[1061] = "Estonian",
		[1080] = "Faroese",
		[1065] = "Farsi",
		[1124] = "Filipino",
		[1035] = "Finnish",
		[1036] = "French - France",
		[2060] = "French - Belgium",
		[11276] = "French - Cameroon",
		[3084] = "French - Canada",
		[9228] = "French - Democratic Rep. of Congo",
		[12300] = "French - Cote d'Ivoire",
		[15372] = "French - Haiti",
		[5132] = "French - Luxembourg",
		[13324] = "French - Mali",
		[6156] = "French - Monaco",
		[14348] = "French - Morocco",
		[58380] = "French - North Africa",
		[8204] = "French - Reunion",
		[10252] = "French - Senegal",
		[4108] = "French - Switzerland",
		[7180] = "French - West Indies",
		[1122] = "French - West Indies",
		[1127] = "Fulfulde - Nigeria",
		[1071] = "FYRO Macedonian",
		[1110] = "Galician",
		[1079] = "Georgian",
		[1031] = "German - Germany",
		[3079] = "German - Austria",
		[5127] = "German - Liechtenstein",
		[4103] = "German - Luxembourg",
		[2055] = "German - Switzerland",
		[1032] = "Greek",
		[1135] = "Greenlandic",
		[1140] = "Guarani - Paraguay",
		[1095] = "Gujarati",
		[1128] = "Hausa - Nigeria",
		[1141] = "Hawaiian - United States",
		[1037] = "Hebrew",
		[1081] = "Hindi",
		[1038] = "Hungarian",
		[1129] = "Ibibio - Nigeria",
		[1039] = "Icelandic",
		[1136] = "Igbo - Nigeria",
		[1057] = "Indonesian",
		[1117] = "Inuktitut",
		[2108] = "Irish",
		[1040] = "Italian - Italy",
		[2064] = "Italian - Switzerland",
		[1041] = "Japanese",
		[1158] = "K'iche",
		[1099] = "Kannada",
		[1137] = "Kanuri - Nigeria",
		[2144] = "Kashmiri",
		[1120] = "Kashmiri (Arabic)",
		[1087] = "Kazakh",
		[1107] = "Khmer",
		[1159] = "Kinyarwanda",
		[1111] = "Konkani",
		[1042] = "Korean",
		[1088] = "Kyrgyz (Cyrillic)",
		[1108] = "Lao",
		[1142] = "Latin",
		[1062] = "Latvian",
		[1063] = "Lithuanian",
		[1134] = "Luxembourgish",
		[1086] = "Malay - Malaysia",
		[2110] = "Malay - Brunei Darussalam",
		[1100] = "Malayalam",
		[1082] = "Maltese",
		[1112] = "Manipuri",
		[1153] = "Maori - New Zealand",
		[1146] = "Mapudungun",
		[1102] = "Marathi",
		[1148] = "Mohawk",
		[1104] = "Mongolian (Cyrillic)",
		[2128] = "Mongolian (Mongolian)",
		[1121] = "Nepali",
		[2145] = "Nepali - India",
		[1044] = "Norwegian (Bokmal)",
		[2068] = "Norwegian (Nynorsk)",
		[1154] = "Occitan",
		[1096] = "Oriya",
		[1138] = "Oromo",
		[1145] = "Papiamentu",
		[1123] = "Pashto",
		[1045] = "Polish",
		[1046] = "Portuguese - Brazil",
		[2070] = "Portuguese - Portugal",
		[1094] = "Punjabi",
		[2118] = "Punjabi (Pakistan)",
		[1131] = "Quecha - Bolivia",
		[2155] = "Quecha - Ecuador",
		[3179] = "Quecha - Peru	CB",
		[1047] = "Rhaeto-Romanic",
		[1048] = "Romanian",
		[2072] = "Romanian - Moldava",
		[1049] = "Russian",
		[2073] = "Russian - Moldava",
		[1083] = "Sami (Lappish)",
		[1103] = "Sanskrit",
		[1084] = "Scottish Gaelic",
		[1132] = "Sepedi",
		[3098] = "Serbian (Cyrillic)",
		[2074] = "Serbian (Latin)",
		[1113] = "Sindhi - India",
		[2137] = "Sindhi - Pakistan",
		[1115] = "Sinhalese - Sri Lanka",
		[1051] = "Slovak",
		[1060] = "Slovenian",
		[1143] = "Somali",
		[1070] = "Sorbian",
		[3082] = "Spanish - Spain (Modern Sort)",
		[1034] = "Spanish - Spain (Traditional Sort)",
		[11274] = "Spanish - Argentina",
		[16394] = "Spanish - Bolivia",
		[13322] = "Spanish - Chile",
		[9226] = "Spanish - Colombia",
		[5130] = "Spanish - Costa Rica",
		[7178] = "Spanish - Dominican Republic",
		[12298] = "Spanish - Ecuador",
		[17418] = "Spanish - El Salvador",
		[4106] = "Spanish - Guatemala",
		[18442] = "Spanish - Honduras",
		[22538] = "Spanish - Latin America",
		[2058] = "Spanish - Mexico",
		[19466] = "Spanish - Nicaragua",
		[6154] = "Spanish - Panama",
		[15370] = "Spanish - Paraguay",
		[10250] = "Spanish - Peru",
		[20490] = "Spanish - Puerto Rico",
		[21514] = "Spanish - United States",
		[14346] = "Spanish - Uruguay",
		[8202] = "Spanish - Venezuela",
		[1072] = "Sutu",
		[1089] = "Swahili",
		[1053] = "Swedish",
		[2077] = "Swedish - Finland",
		[1114] = "Syriac",
		[1064] = "Tajik",
		[1119] = "Tamazight (Arabic)",
		[2143] = "Tamazight (Latin)",
		[1097] = "Tamil",
		[1092] = "Tatar",
		[1098] = "Telugu",
		[1054] = "Thai",
		[2129] = "Tibetan - Bhutan",
		[1105] = "Tibetan - People's Republic of China",
		[2163] = "Tigrigna - Eritrea",
		[1139] = "Tigrigna - Ethiopia",
		[1073] = "Tsonga",
		[1074] = "Tswana",
		[1055] = "Turkish",
		[1090] = "Turkmen",
		[1152] = "Uighur - China",
		[1058] = "Ukrainian",
		[1056] = "Urdu",
		[2080] = "Urdu - India",
		[2115] = "Uzbek (Cyrillic)",
		[1091] = "Uzbek (Latin)",
		[1075] = "Venda",
		[1066] = "Vietnamese",
		[1106] = "Welsh",
		[1160] = "Wolof",
		[1076] = "Xhosa",
		[1157] = "Yakut",
		[1144] = "Yi",
		[1085] = "Yiddish",
		[1130] = "Yoruba",
		[1077] = "Zulu",
		[1279] = "HID (Human Interface Device)",
	} &default = function(n: count): string { return fmt("keyboard-%d", n); };
}
