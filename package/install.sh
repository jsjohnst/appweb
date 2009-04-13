#!/bin/bash
#
#   install: Installation script
#
#   Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
#   Usage: install [configFile]
#
################################################################################
#
#   The configFile is of the format:
#       FMT=[rpm|deb|tar]               # Package format to use
#       srcDir=sourcePath               # Where to install the doc
#       devDir=documentationPath        # Where to install the doc
#       installbin=[YN]                 # Install binary package
#       installsrc=[YN]                 # Install source code package
#       installdev=[YN]                 # Install dev headers package
#       runDaemon=[YN]                  # Run the program as a daemon
#       httpPort=portNumber             # Http port to listen on
#       sslPort=portNumber              # SSL port to listen on
#       username=username               # User account for appweb
#       groupname=groupname             # Group account for appweb
#       hostname=groupname              # Serving host
#

HOME=`pwd`
FMT=
SITE=127.0.0.1
PAGE=/index.html

HOSTNAME=`hostname`
BLD_COMPANY="!!BLD_COMPANY!!"
BLD_PRODUCT="!!BLD_PRODUCT!!"
BLD_NAME="!!BLD_NAME!!"
BLD_VERSION="!!BLD_VERSION!!"
BLD_NUMBER="!!BLD_NUMBER!!"
BLD_HOST_OS="!!BLD_HOST_OS!!"
BLD_HOST_CPU="!!BLD_HOST_CPU!!"
BLD_HOST_DIST="!!BLD_HOST_DIST!!"

BLD_PREFIX="!!BLD_PREFIX!!"             # Fixed and can't be relocated
BLD_DOC_PREFIX="!!BLD_DOC_PREFIX!!"
BLD_INC_PREFIX="!!BLD_INC_PREFIX!!"
BLD_LIB_PREFIX="!!BLD_LIB_PREFIX!!"
BLD_MOD_PREFIX="!!BLD_MOD_PREFIX!!"
BLD_SAM_PREFIX="!!BLD_SAM_PREFIX!!"
BLD_SBIN_PREFIX="!!BLD_SBIN_PREFIX!!"
BLD_SRC_PREFIX="!!BLD_SRC_PREFIX!!"
BLD_WEB_PREFIX="!!BLD_WEB_PREFIX!!"

installbin=Y
installdev=Y
installsrc=N
runDaemon=Y
HTTP_PORT=7777
SSL_PORT=4443

PATH=$PATH:/sbin:/usr/sbin

###############################################################################
#
#   Initialization
#

setup() {

    umask 022

    if [ $BLD_HOST_OS != WIN -a `id -u` != "0" ] ; then
        echo "You must be root to install this product."
        exit 255
    fi

    #
    #   Headless install
    #
    if [ $# -ge 1 ] ; then
        if [ ! -f $1 ] ; then
            echo "Could not find installation config file \"$1\"." 1>&2
            exit 255
        else
            . $1 
            installFiles $FMT
            
            if [ "$installbin" = "Y" ] ; then
                patchConfiguration
                if [ "$runDaemon" = "Y" ] ; then
                    startService
                    startBrowser
                fi
            fi

        fi
        exit 0
    fi

    sleuthPackageFormat

    getAccountDetails

    echo -e "\n$BLD_NAME !!BLD_VERSION!!-!!BLD_NUMBER!! Installation\n"

}


getAccountDetails() {

    local g u

    #
    #   Select default username
    #
    for u in nobody www-data Administrator 
    do
        grep "$u" /etc/passwd >/dev/null
        if [ $? = 0 ] ; then
            username=$u
            break
        fi
    done

    if [ "$username" = "" ] ; then
        echo "Can't find a suitable username in /etc/passwd for $BLD_PRODUCT" 1>&2
        exit 255
    fi
    
    #
    #   Select default group name
    #
    for g in nobody nogroup www-data Administrators
    do
        grep "$g" /etc/group >/dev/null
        if [ $? = 0 ] ; then
            groupname=$g
            break
        fi
    done
    
    if [ "$groupname" = "" ] ; then
        echo "Can't find a suitable group in /etc/group for $BLD_PRODUCT" 1>&2
        exit 255
    fi
}


#
#   Try to guess if we should default to using RPM
#

sleuthPackageFormat() {
    local name

    name=`createPackageName ${BLD_PRODUCT}-bin`
    FMT=
    for f in deb rpm tar.gz ; do
        if [ -f ${name}.${f} ] ; then
            FMT=${f%.gz}
            break
        fi
    done

    if [ "$FMT" = "" ] ; then
        echo -e "\nYou may be be missing a necessary package file. "
        echo "Check that you have the correct $BLD_NAME package".
        exit 255
    fi
}



askUser() {
    local finished

    echo "Enter requested configuration information or press <ENTER> to accept"
    echo -e "the defaults. "
    
    #
    #   Confirm the configuration
    #
    finished=N
    while [ "$finished" = "N" ]
    do
        echo
        installbin=`yesno "Install binary package" "$installbin"`
        installdev=`yesno "Install development headers, samples and documentation package" "$installdev"`
#       installsrc=`yesno "Install source package" "$installsrc"`
    
        if [ "$installbin" = "Y" ] ; then
            runDaemon=`yesno "Start $BLD_PRODUCT automatically at system boot" $runDaemon`
            HTTP_PORT=`ask "Enter the HTTP port number" "$HTTP_PORT"`
            SSL_PORT=`ask "Enter the SSL port number" "$SSL_PORT"`
            username=`ask "Enter the user account for $BLD_PRODUCT" "$username"`
            groupname=`ask "Enter the user group for $BLD_PRODUCT" "$groupname"`
        else
            runDaemon=N
        fi
    
        echo -e "\nInstalling with this configuration:" 
        echo -e "    Install binary package: $installbin"
        echo -e "    Install development headers and samples package: $installdev"
#       echo -e "    Install source package: $installsrc"
    
        if [ "$installbin" = "Y" ] ; then
            echo -e "    Start automatically at system boot: $runDaemon"
            echo -e "    HTTP port number: $HTTP_PORT"
            echo -e "    SSL port number: $SSL_PORT"
            echo -e "    Username: $username"
            echo -e "    Groupname: $groupname"
        fi
    
        echo
        finished=`yesno "Accept this configuration" "Y"`
    done
    
    if [ "$installbin" = "N" -a "$installdev" = "N" -a \ "$installsrc" = "N" ] ; then
        echo -e "\nNothing to install, exiting. "
        exit 0
    fi
    
    #
    #   Save the install settings. Remove.sh will need this
    #
    saveSetup
}


createPackageName() {

    echo ${1}-${BLD_VERSION}-${BLD_NUMBER}-${BLD_HOST_DIST}-${BLD_HOST_OS}-${BLD_HOST_CPU}
}


# 
#   Get a yes/no answer from the user. Usage: ans=`yesno "prompt" "default"`
#   Echos 1 for Y or 0 for N
#
yesno() {
    local ans

    if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
        echo "Y"
        return
    fi
    echo -n "$1 [$2] : " 1>&2
    while [ 1 ] 
    do
        read ans
        if [ "$ans" = "" ] ; then
            echo $2 ; break
        elif [ "$ans" = "Y" -o "$ans" = "y" ] ; then
            echo "Y" ; break
        elif [ "$ans" = "N" -o "$ans" = "n" ] ; then
            echo "N" ; break
        fi
        echo -e "\nMust enter a 'y' or 'n'\n " 1>&1
    done
}


# 
#   Get input from the user. Usage: ans=`ask "prompt" "default"`
#   Returns the answer or default if <ENTER> is pressed
#
ask() {
    local ans

    default=$2

    if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
        echo "$default"
        return
    fi

    echo -n "$1 [$default] : " 1>&2
    read ans
    if [ "$ans" = "" ] ; then
        echo $default
    fi
    echo $ans
}


#
#   Save the setup
#
saveSetup() {
    local firstChar

    echo -e "FMT=$FMT\nbinDir=$BLD_PREFIX\ninstallbin=$installbin\ninstalldev=$installdev\ninstallsrc=$installsrc\nrunDaemon=$runDaemon\nhttpPort=$HTTP_PORT\nsslPort=$SSL_PORT\nusername=$username\ngroupname=$groupname\nhostname=$HOSTNAME" >/etc/${BLD_PRODUCT}Install.conf
}


#
#   Copy of the script in build/bin/patchAppwebConf
#
patchAppwebConf()
{
    conf="$1"
    ssl="$2"
    log="$3"
    misc="$4"
    doc="$5"
    docPrefix="${BLD_DOC_PREFIX}"
    
    ed -s "$conf" <<!PATCH_EOF
    H
    ,g!^ServerRoot!s!\".*\"!\"${BLD_PREFIX}\"!
    ,g!DocumentRoot!s!\".*\"!\"${BLD_WEB_PREFIX}\"!
    ,g!LoadModulePath .*!s!!LoadModulePath \"${BLD_MOD_PREFIX}\"!
    1;/Listen/;s!^Listen .*!Listen ${BLD_HTTP_PORT}!
    w
    q
!PATCH_EOF

    ed -s "$ssl" <<!PATCH_EOF2
    H
    1;/Listen/;s!^.*Listen .*!    Listen ${BLD_SSL_PORT}!
    ,g!DocumentRoot!s!\".*\"!\"${BLD_WEB_PREFIX}\"!
    1;/SSLEngine on/;?<VirtualHost?;s!<VirtualHost .*!<VirtualHost *:${BLD_SSL_PORT}>!
    w
    q
!PATCH_EOF2
    
    ed -s "$log" <<!PATCH_EOF3
    H
    ,g!ErrorLog .*!s!!ErrorLog \"${BLD_LOG_PREFIX}/error.log\"!
    ,g!CustomLog .*access.log\"!s!!CustomLog \"${BLD_LOG_PREFIX}/access.log\"!
    w
    q
!PATCH_EOF3

    ed -s "$misc" <<!PATCH_EOF4
    H
    ,g!User .*!s!!User ${username}!
    ,g!Group .*!s!!Group ${groupname}!
    w
    q
!PATCH_EOF4

    ed -s "$doc" <<!PATCH_EOF5
    H
    ,g!Alias \/doc\/!s!\".*\"!\"${docPrefix}\/\"!
    w
    q
!PATCH_EOF5

    if [ `uname -s` = CYGWIN_NT-5.1 ] ; then
        if which unix2dos >/dev/null 2>&1 ; then
            unix2dos "$conf" >/dev/null 2>&1
            unix2dos "$ssl" >/dev/null 2>&1
            unix2dos "$log" >/dev/null 2>&1
            unix2dos "$misc" >/dev/null 2>&1
            unix2dos "$doc" >/dev/null 2>&1
        fi
    fi
}


#
#   Modify service.  
#   Usage: configureService [start|stop|install|remove]
#
configureService() {
    local action=$1

    case $action in
    start|stop)
        if [ $BLD_HOST_OS = WIN ] ; then
            if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
                if [ $action = start ] ; then
                    "$BLD_SBIN_PREFIX/$BLD_PRODUCT" -g
                else 
                    "$BLD_SBIN_PREFIX/$BLD_PRODUCT" -s
                fi
            fi
        elif which launchctl >/dev/null 2>&1 ; then
            local company=`echo $BLD_COMPANY | tr '[A-Z]' '[a-z']`
            if [ $action = start ] ; then
                launchctl start com.${company}.${BLD_PRODUCT}
            else
                launchctl stop com.${company}.${BLD_PRODUCT} 2>/dev/null
            fi
        elif which service >/dev/null 2>&1 ; then
            /sbin/service $BLD_PRODUCT $action >/dev/null 2>&1
        elif which invoke-rc.d >/dev/null 2>&1 ; then
            invoke-rc.d $BLD_PRODUCT $action
        fi
        ;;

    install)
        if [ $BLD_HOST_OS = WIN ] ; then
            if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
                "$BLD_SBIN_PREFIX/$BLD_PRODUCT" -i default
            fi
        elif which launchctl >/dev/null 2>&1 ; then
            local company=`echo $BLD_COMPANY | tr '[A-Z]' '[a-z']`
            launchctl unload /Library/LaunchDaemons/com.${company}.${BLD_PRODUCT}.plist 2>/dev/null
            launchctl load -w /Library/LaunchDaemons/com.${company}.${BLD_PRODUCT}.plist
        elif which chkconfig >/dev/null 2>&1 ; then
            /sbin/chkconfig --add $BLD_PRODUCT >/dev/null
            /sbin/chkconfig --level 5 $BLD_PRODUCT on >/dev/null

        elif which update-rc.d >/dev/null 2>&1 ; then
            update-rc.d $BLD_PRODUCT defaults >/dev/null
        fi
        ;;

    remove)
        if [ $BLD_HOST_OS = WIN ] ; then
            if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
                "$BLD_SBIN_PREFIX/$BLD_PRODUCT" -u
            fi
        elif which launchctl >/dev/null 2>&1 ; then
            local company=`echo $BLD_COMPANY | tr '[A-Z]' '[a-z']`
            launchctl unload -w /Library/LaunchDaemons/com.${company}.${BLD_PRODUCT}.plist 2>/dev/null
        elif which chkconfig >/dev/null 2>&1 ; then
            /sbin/chkconfig --del $BLD_PRODUCT >/dev/null
        elif which update-rc.d >/dev/null 2>&1 ; then
            update-rc.d -f $BLD_PRODUCT remove >/dev/null
        fi
        ;;
    esac
}



installFiles() {
    local dir pkg doins NAME upper

    echo -e "\nExtracting files ..."

    for pkg in bin dev src ; do
        
        doins=`eval echo \\$install${pkg}`
        if [ "$doins" = Y ] ; then
            upper=`echo $pkg | tr '[a-z]' '[A-Z]'`
            suffix="-${pkg}"
            #
            #   RPM doesn't give enough control on error codes. So best to
            #   keep going. 
            #
            NAME=`createPackageName ${BLD_PRODUCT}${suffix}`.$FMT
            if [ "$FMT" = "rpm" ] ; then
                echo -e "\nrpm -Uhv $NAME"
                rpm -Uhv $HOME/$NAME
            elif [ "$FMT" = "deb" ] ; then
                echo -e "\ndpkg -i $NAME"
                dpkg -i $HOME/$NAME >/dev/null
            else
                # cd /
                # echo -e "\ntar xfz ${NAME}.gz"
                # tar xfz $HOME/${NAME}.gz

                #
                #   Need to strip the top directory off. Not all tar versions
                #   support tar --strip=N yet
                #
                dir=/tmp/${BLD_PRODUCT}-$$
                mkdir -p $dir
                cd $dir
                echo -e "\ntar xfz ${NAME}.gz"
                tar xfz $HOME/${NAME}.gz
                cd "${upper}" >/dev/null
                #
                #   Need to avoid any directories for redhat9 and so we don't change perms unnecessarily.
                #
                ( find . -type f ; find . -type l) | xargs tar cf -  | tar xpf - -C /
                cd $HOME
                rm -fr $dir
            fi
        fi
    done

    if [ -f /etc/redhat-release -a -x /usr/bin/chcon ] ; then 
        if sestatus | grep enabled >/dev/nulll ; then
            for f in $BLD_LIB_PREFIX/*.so $BLD_MOD_PREFIX/*.so ; do
                chcon /usr/bin/chcon -t texrel_shlib_t $f
            done
        fi
    fi

    if which ldconfig >/dev/null 2>&1 ; then
        ldconfig /usr/lib/lib$BLD_PRODUCT.so.?.?.?
        ldconfig -n /usr/lib/$BLD_PRODUCT
        ldconfig -n /usr/lib/$BLD_PRODUCT/modules
    fi

    if [ $BLD_HOST_OS = WIN ] ; then
        echo -e "\nSetting file permissions ..."
        find "$BLD_PREFIX" -type d | xargs chmod 755 
        find "$BLD_PREFIX" -type f | xargs chmod g+r,o+r 
        find "$BLD_WEB_PREFIX" -type d | xargs chmod 755
        find "$BLD_WEB_PREFIX" -type f | xargs chmod g+r,o+r
        chmod 777 "$BLD_PREFIX/logs"
        chmod 755 "$BLD_WEB_PREFIX" "$BLD_WEB_PREFIX"/..
    fi

    echo
}


patchConfiguration() {

    if [ ! -f $BLD_PRODUCT.conf -a -f "$BLD_PREFIX/new.conf" ] ; then
        cp "$BLD_PREFIX/new.conf" "$BLD_PREFIX/$BLD_PRODUCT.conf"
    fi

    BLD_PREFIX="$BLD_PREFIX" 
        BLD_WEB_PREFIX="!!BLD_WEB_PREFIX!!" \
        BLD_DOC_PREFIX="$BLD_DOC_PREFIX" \
        BLD_LIB_PREFIX="!!BLD_LIB_PREFIX!!" \
        BLD_MOD_PREFIX="!!BLD_MOD_PREFIX!!" \
        BLD_SERVER=$HOSTNAME \
        BLD_LOG_PREFIX="!!BLD_LOG_PREFIX!!" \
        BLD_HTTP_PORT=$HTTP_PORT \
        BLD_SSL_PORT=$SSL_PORT \
        patchAppwebConf ${BLD_PREFIX}/appweb.conf ${BLD_PREFIX}/conf/hosts/ssl-default.conf ${BLD_PREFIX}/conf/log.conf \
            ${BLD_PREFIX}/conf/misc.conf ${BLD_PREFIX}/conf/extras/doc.conf
}


startService() {

    configureService stop >/dev/null 2>&1
    configureService remove >/dev/null 2>&1

    configureService install
    configureService start
}


startBrowser() {

    if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
        return
    fi
    #
	#	Conservative delay to allow appweb to start and initialize
	#
    sleep 5
    if [ $BLD_HOST_OS = WIN ] ; then
        echo -e "\nStarting browser to view the $BLD_NAME Home Page."
        cmd /c start /max /wait http://$SITE:$HTTP_PORT$PAGE
    elif [ $BLD_HOST_OS = MACOSX ] ; then
        open http://$SITE:$HTTP_PORT$PAGE >/dev/null 2>&1 &
    else
        for f in /usr/bin/htmlview /usr/bin/firefox /usr/bin/mozilla \
            /usr/bin/konqueror
        do
            if [ -x ${f} ] ; then
                echo -e "\nStarting browser to view the $BLD_NAME Home Page."
                sudo -H -b ${f} http://$SITE:$HTTP_PORT$PAGE >/dev/null 2>&1 &
                break
            fi
        done
    fi
}

#
#   Main program for install script
#

setup $*

askUser

installFiles $FMT

if [ "$installbin" = "Y" ] ; then
    patchConfiguration
    if [ "$runDaemon" = "Y" ] ; then
        startService
        startBrowser
    fi
fi

echo -e "\n$BLD_NAME installation successful.\n"
