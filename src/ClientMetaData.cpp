#include "ClientMetaData.h"

namespace metadata
{
    const char* const CRM_META_DATA_XML =
        "<resource-agent name=\"fence-universal\"\n"
        "  shortdesc=\"Universal Fencing Hub client\">\n"
        "  <longdesc>\n"
        "  </longdesc>\n"
        "  <vendor-url>\n"
        "  </vendor-url>\n"
        "  <parameters>\n"
        "    <parameter name=\"action\" unique=\"1\" required=\"1\">\n"
        "      <content type=\"string\" default=\"off\"/>\n"
        "      <shortdesc lang=\"en\">\n"
        "        Fencing action to perform: off, reboot, on\n"
        "      </shortdesc>\n"
        "    </parameter>\n"
        "    <parameter name=\"protocol\" unique=\"1\" required=\"1\">\n"
        "      <content type=\"string\"/>\n"
        "      <shortdesc lang=\"en\">\n"
        "        Protocol for the connection to the Universal Fencing Hub server: IPV4, IPV6\n"
        "      </shortdesc>\n"
        "    </parameter>\n"
        "    <parameter name=\"ip_address\" unique=\"1\" required=\"1\">\n"
        "      <content type=\"string\"/>\n"
        "      <shortdesc lang=\"en\">\n"
        "        IP address of the Universal Fencing Hub server\n"
        "      </shortdesc>\n"
        "    </parameter>\n"
        "    <parameter name=\"tcp_port\" unique=\"1\" required=\"1\">\n"
        "      <content type=\"string\"/>\n"
        "        TCP port address of the Universal Fencing Hub server\n"
        "      <shortdesc lang=\"en\">\n"
        "      </shortdesc>\n"
        "    </parameter>\n"
        "    <parameter name=\"secret\" unique=\"1\" required=\"1\">\n"
        "      <content type=\"string\"/>\n"
        "      <shortdesc lang=\"en\">\n"
        "        Password for sign in to the Univseral Fencing Hub server\n"
        "      </shortdesc>\n"
        "    </parameter>\n"
        "  </parameters>\n"
        "  <actions>\n"
        "    <action name=\"off\"/>\n"
        "    <action name=\"reboot\"/>\n"
        "    <action name=\"on\"/>\n"
        "    <action name=\"metadata\"/>\n"
        "    <action name=\"status\"/>\n"
        "    <action name=\"list\"/>\n"
        "    <action name=\"monitor\"/>\n"
        "    <action name=\"start\" timeout=\"20s\"/>\n"
        "    <action name=\"stop\" timeout=\"20s\"/>\n"
        "  </actions>\n"
        "</resource-agent>\n";
}
