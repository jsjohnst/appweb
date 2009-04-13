#!ejs
/*
 *  Patch an appweb configuration files. This patches appweb.conf and conf/hosts/ssl-default.conf
 *
 *  usage: patchConf settings.log
 *
 *  Where the settings.log file contains
 *
 *  { port: PORT, ssl: SSL_PORT, web: DOCUMENT_ROOT, root: SERVER_ROOT }
 */

settings = App.args[1]
str = File.getString(settings)
obj = deserialize(str)
obj.root = obj.root.replace(/\\/g, '/')
obj.web = obj.web.replace(/\\/g, '/')

function save(name: String, data: String) {
    rm(name + ".new")
    File.put(name + ".new", 0644, data)
    rm(name + ".bak")
    cp(name, name + ".bak")
    mv(name + ".new", name)
}

/*
 *  Patch appweb.conf. Patch the port, server root and document root
 */
name = "appweb.conf"
conf = File.getString(name)
conf = conf.replace(/7777/g, obj.port)
conf = conf.replace(/ServerRoot.*/g, 'ServerRoot "' + obj.root + '"\r')
conf = conf.replace(/DocumentRoot.*/g, 'DocumentRoot "' + obj.web + '"\r')
conf = conf.replace(/LoadModulePath.*/g, 'LoadModulePath "' + obj.root + '/bin"\r')
conf = conf.replace(/	/g, '    ')
save(name, conf)

/*
 *   Patch conf/hosts/ssl-default.conf. Patch the SSL port and document root.
 */
name = "conf/hosts/ssl-default.conf"
conf = File.getString(name)
conf = conf.replace(/4443/g, obj.ssl)
conf = conf.replace(/DocumentRoot.*/g, 'DocumentRoot "' + obj.web + '"\r')
conf = conf.replace(/	/g, '    ')
save(name, conf)

/*
 *   conf/log.conf
 */
name = "conf/log.conf"
conf = File.getString(name)
conf = conf.replace(/ErrorLog.*/g, 'ErrorLog "' + obj.root + '/logs/error.log"\r')
conf = conf.replace(/CustomLog.*access.log"/g, 'CustomLog "' + obj.root + '/logs/access.log"')
conf = conf.replace(/	/g, '    ')
save(name, conf)

/*
 *   conf/extras/doc.conf
 */
name = "conf/extras/doc.conf"
conf = File.getString(name)
conf = conf.replace(/Alias \/doc\/.*/g, 'Alias /doc/ "' + obj.root + '/doc/"\r')
conf = conf.replace(/	/g, '    ')
save(name, conf)
