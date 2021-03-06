import urllib
import os
import openAnything
from xml.dom import minidom

def parseShow(seriesID, show_name):
    safe_show_name = show_name.replace(":", "")
    details_url = "http://thetvdb.com/api/EB49E8B9E78EBEE1/series/"+seriesID+"/all/en.xml"
    details = openAnything.fetch(details_url)
    details_xml = minidom.parseString(details['data'])
    seasons = details_xml.getElementsByTagName("SeasonNumber")
    episodes = details_xml.getElementsByTagName("EpisodeNumber")
    # check to see if parent show path needs to be made
    if not os.access(safe_show_name, os.F_OK):
        os.makedirs(safe_show_name)
    i = 0
    for item in episodes:
        season = seasons[i].firstChild.data
        episode = item.firstChild.data
        filename = safe_show_name+" S"+season+"E"+episode+".avi"
        # seeif season path exists or not, and make it if not
        if os.access(safe_show_name + "\\Season " + season, os.F_OK):
            # just go ahead and create the file
            file = open(safe_show_name + "\\Season " + season + "\\" + filename, "w")
            file.close()
        else:
            os.makedirs(safe_show_name + "\\Season " + season)
            file = open(safe_show_name + "\\Season " + season + "\\" + filename, "w")
            file.close()
        print "Creating %s" % filename
        i = i + 1
        
show_file = open("shows.txt")
shows = show_file.read().split("\n")
show_file.close()
for item in shows:
    show_url = "http://thetvdb.com/api/GetSeries.php?"+urllib.urlencode({"seriesname":item})
    print "Building "+item+"..."
    show_xml = openAnything.fetch(show_url)
    xmldoc = minidom.parseString(show_xml['data'])
    node = xmldoc.getElementsByTagName("seriesid")
    if ("node" in dir()):
        seriesID = node[0].firstChild.data
        parseShow(seriesID, item)
    else:
        print "Could not find any data for "+show_name+" on TVDB.\nURL: "+show_url
