/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef APACHE_CONFIGURATION_HH
#define APACHE_CONFIGURATION_HH

std::string apache_main_config = "Timeout 300\nKeepAlive On\nMaxKeepAliveRequests 100\nKeepAliveTimeout 5\nLogLevel warn\nLoadModule dir_module /usr/lib/apache2/modules/mod_dir.so\n<IfModule mod_dir.c>\nDirectoryIndex index.html index.cgi index.pl index.php index.xhtml index.htm\n</IfModule>\nLoadModule mime_module /usr/lib/apache2/modules/mod_mime.so\n<IfModule mod_mime.c>\nTypesConfig /etc/mime.types\nAddType application/x-compress .Z\nAddType application/x-gzip .gz .tgz\nAddType application/x-bzip2 .bz2\nAddLanguage am .amh\nAddLanguage ar .ara\nAddLanguage be .be\nAddLanguage bg .bg\nAddLanguage bn .bn\nAddLanguage br .br\nAddLanguage bs .bs\nAddLanguage ca .ca\nAddLanguage cs .cz .cs\nAddLanguage cy .cy\nAddLanguage da .dk\nAddLanguage de .de\nAddLanguage dz .dz\nAddLanguage el .el\nAddLanguage en .en\nAddLanguage eo .eo\nRemoveType  es\nAddLanguage es .es\nAddLanguage et .et\nAddLanguage eu .eu\nAddLanguage fa .fa\nAddLanguage fi .fi\nAddLanguage fr .fr\nAddLanguage ga .ga\nAddLanguage gl .glg\nAddLanguage gu .gu\nAddLanguage he .he\nAddLanguage hi .hi\nAddLanguage hr .hr\nAddLanguage hu .hu\nAddLanguage hy .hy\nAddLanguage id .id\nAddLanguage is .is\nAddLanguage it .it\nAddLanguage ja .ja\nAddLanguage ka .ka\nAddLanguage kk .kk\nAddLanguage km .km\nAddLanguage kn .kn\nAddLanguage ko .ko\nAddLanguage ku .ku\nAddLanguage lo .lo\nAddLanguage lt .lt\nAddLanguage ltz .ltz\nAddLanguage lv .lv\nAddLanguage mg .mg\nAddLanguage mk .mk\nAddLanguage ml .ml\nAddLanguage mr .mr\nAddLanguage ms .msa\nAddLanguage nb .nob\nAddLanguage ne .ne\nAddLanguage nl .nl\nAddLanguage nn .nn\nAddLanguage no .no\nAddLanguage pa .pa\nAddLanguage pl .po\nAddLanguage pt-BR .pt-br\nAddLanguage pt .pt\nAddLanguage ro .ro\nAddLanguage ru .ru\nAddLanguage sa .sa\nAddLanguage se .se\nAddLanguage si .si\nAddLanguage sk .sk\nAddLanguage sl .sl\nAddLanguage sq .sq\nAddLanguage sr .sr\nAddLanguage sv .sv\nAddLanguage ta .ta\nAddLanguage te .te\nAddLanguage th .th\nAddLanguage tl .tl\nRemoveType  tr\nAddLanguage tr .tr\nAddLanguage uk .uk\nAddLanguage ur .ur\nAddLanguage vi .vi\nAddLanguage wo .wo\nAddLanguage xh .xh\nAddLanguage zh-CN .zh-cn\nAddLanguage zh-TW .zh-tw\nAddCharset us-ascii     .ascii .us-ascii\nAddCharset ISO-8859-1  .iso8859-1  .latin1\nAddCharset ISO-8859-2  .iso8859-2  .latin2 .cen\nAddCharset ISO-8859-3  .iso8859-3  .latin3\nAddCharset ISO-8859-4  .iso8859-4  .latin4\nAddCharset ISO-8859-5  .iso8859-5  .cyr .iso-ru\nAddCharset ISO-8859-6  .iso8859-6  .arb .arabic\nAddCharset ISO-8859-7  .iso8859-7  .grk .greek\nAddCharset ISO-8859-8  .iso8859-8  .heb .hebrew\nAddCharset ISO-8859-9  .iso8859-9  .latin5 .trk\nAddCharset ISO-8859-10  .iso8859-10  .latin6\nAddCharset ISO-8859-13  .iso8859-13\nAddCharset ISO-8859-14  .iso8859-14  .latin8\nAddCharset ISO-8859-15  .iso8859-15  .latin9\nAddCharset ISO-8859-16  .iso8859-16  .latin10\nAddCharset ISO-2022-JP .iso2022-jp .jis\nAddCharset ISO-2022-KR .iso2022-kr .kis\nAddCharset ISO-2022-CN .iso2022-cn .cis\nAddCharset Big5         .Big5      .big5 .b5\nAddCharset cn-Big5       .cn-big5\nAddCharset WINDOWS-1251 .cp-1251   .win-1251\nAddCharset CP866           .cp866\nAddCharset KOI8   .koi8\nAddCharset KOI8-E         .koi8-e\nAddCharset KOI8-r         .koi8-r .koi8-ru\nAddCharset KOI8-U         .koi8-u\nAddCharset KOI8-ru       .koi8-uk .ua\nAddCharset ISO-10646-UCS-2 .ucs2\nAddCharset ISO-10646-UCS-4 .ucs4\nAddCharset UTF-7           .utf7\nAddCharset UTF-8           .utf8\nAddCharset UTF-16         .utf16\nAddCharset UTF-16BE     .utf16be\nAddCharset UTF-16LE     .utf16le\nAddCharset UTF-32         .utf32\nAddCharset UTF-32BE     .utf32be\nAddCharset UTF-32LE     .utf32le\nAddCharset euc-cn         .euc-cn\nAddCharset euc-gb         .euc-gb\nAddCharset euc-jp         .euc-jp\nAddCharset euc-kr         .euc-kr\nAddCharset EUC-TW         .euc-tw\nAddCharset gb2312         .gb2312 .gb\nAddCharset iso-10646-ucs-2 .ucs-2 .iso-10646-ucs-2\nAddCharset iso-10646-ucs-4 .ucs-4 .iso-10646-ucs-4\nAddCharset shift_jis   .shift_jis .sjis\nAddCharset BRF           .brf\nAddHandler type-map var\nAddType text/html .shtml\nAddOutputFilter INCLUDES .shtml\nAddHandler cgi-script .cgi\n</IfModule>\nLogFormat \"%h %l %u %t \\\"%r\\\" %>s %O\" common\nLoadModule authz_host_module /usr/lib/apache2/modules/mod_authz_host.so\nLoadModule cgi_module /usr/lib/apache2/modules/mod_cgi.so\nLoadModule alias_module /usr/lib/apache2/modules/mod_alias.so\nScriptAlias / /usr/local/bin/nph-replayserver.cgi\n<Directory /usr/local/bin/>\nAllowOverride None\nOptions +ExecCGI\nOrder allow,deny\n Allow from all\n</Directory>\nLoadModule rewrite_module /usr/lib/apache2/modules/mod_rewrite.so\nRewriteEngine On\nRewriteRule ^(.*)$ /usr/local/bin/nph-replayserver.cgi\nLoadModule env_module /usr/lib/apache2/modules/mod_env.so\nSetEnv RECORD_FOLDER ";

std::string apache_ssl_config = "LoadModule ssl_module /usr/lib/apache2/modules/mod_ssl_with_npn.so\n<IfModule mod_ssl.c>\nSSLRandomSeed startup builtin\nSSLRandomSeed startup file:/dev/urandom 512\nSSLRandomSeed connect builtin\nSSLRandomSeed connect file:/dev/urandom 512\nAddType application/x-x509-ca-cert .crt\nAddType application/x-pkcs7-crl .crl\nSSLPassPhraseDialog exec:/usr/share/apache2/ask-for-passphrase\nSSLSessionCache                 shmcb:/var/run/apache2/ssl_scache(512000)\nSSLSessionCacheTimeout  300\nSSLCipherSuite HIGH:MEDIUM:!aNULL:!MD5\nSSLProtocol all\n</IfModule>\nSSLEngine on\nSSLCertificateFile      /etc/ssl/certs/ssl-cert-snakeoil.pem\nSSLCertificateKeyFile /etc/ssl/private/ssl-cert-snakeoil.key\n";

std::string apache_spdy_config = "LoadModule spdy_module /usr/lib/apache2/modules/mod_spdy.so\nSpdyEnabled on\n";

#endif /* APACHE_CONFIGURATION_HH */
