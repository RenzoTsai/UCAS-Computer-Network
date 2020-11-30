do 
	local p_mospf = Proto("mospf", "mOSPF")

	local f_ospf_ver = ProtoField.uint8("mospf.version", "mOSPF Version", base.DEC)
	local f_ospf_type = ProtoField.uint8("mospf.type", "mOSPF Message Type", base.DEC)
	local f_ospf_length = ProtoField.uint16("mospf.length", "mOSPF Message Length", base.DEC)
	local f_ospf_rid = ProtoField.ipv4("mospf.rid", "Router ID")
	local f_ospf_aid = ProtoField.ipv4("mospf.aid", "Area ID")
	local f_ospf_checksum = ProtoField.uint16("mospf.checksum", "Checksum", base.HEX)
	local f_ospf_padding = ProtoField.uint16("mospf.padding", "Padding", base.HEX)

	p_mospf.fields = { f_ospf_ver, f_ospf_type, f_ospf_length, f_ospf_rid, f_ospf_aid, f_ospf_checksum, f_ospf_padding }

	local p_mospf_hello = Proto("mospf_hello", "mOSPF Hello Message")

	local f_hello_net_mask = ProtoField.ipv4("mospf_hello.mask", "Network Mask of the Interface")
	local f_hello_interval = ProtoField.uint16("mospf_hello.interval", "Interval between Hello Messages", base.DEC)
	local f_hello_padding = ProtoField.uint16("mospf_hello.padding", "Padding of Hello Messages", base.HEX)

	p_mospf_hello.fields = { f_hello_net_mask, f_hello_interval, f_hello_padding }

	local p_mospf_lsu = Proto("mospf_lsu", "mOSPF Link State Update Message")

	local f_lsu_seq = ProtoField.uint16("mospf_lsu.seq", "Sequence Number of LSU Message", base.DEC)
	local f_lsu_ttl = ProtoField.uint8("mospf_lsu.ttl", "TTL of LSU Message", base.DEC)
	local f_lsu_unused = ProtoField.uint8("mospf_lsu.unused", "Unused Byte", base.HEX)
	local f_lsu_nadv = ProtoField.uint32("mospf_lsu.nadv", "Number of Advertisements", base.DEC)

	p_mospf_lsu.fields = { f_lsu_seq, f_lsu_ttl, f_lsu_unused, f_lsu_nadv }

	local p_mospf_lsa = Proto("mospf_lsa", "mOSPF Link State Advertisement")

	local f_lsa_network = ProtoField.ipv4("mospf_lsa.network", "Network of the Interface")
	local f_lsa_mask = ProtoField.ipv4("mospf_lsa.mask", "Net Mask of the Interface")
	local f_lsa_rid = ProtoField.ipv4("mospf_lsa.rid", "Router ID")

	p_mospf_lsa.fields = { f_lsa_network, f_lsa_mask, f_lsa_rid }

	local data_dis = Dissector.get("data")

	function p_mospf.dissector(buf, pkt, root)
		pkt.cols.protocol = "mOSPF"

		local t = root:add(p_mospf, buf(0,16))

		t:add(f_ospf_ver, buf(0,1))
		t:add(f_ospf_type, buf(1,1))
		t:add(f_ospf_length, buf(2,2))
		t:add(f_ospf_rid, buf(4,4))
		t:add(f_ospf_aid, buf(8,4))
		t:add(f_ospf_checksum, buf(12,2))
		t:add(f_ospf_padding, buf(14,2))

		local f_type = buf(1,1):uint()
		if f_type == 1
		then
			pkt.cols.protocol = "mOSPF Hello"
			local hello = root:add(p_mospf_hello, buf(16, 8))
			hello:add(f_hello_net_mask, buf(16,4))
			hello:add(f_hello_interval, buf(20,2))
			hello:add(f_hello_padding, buf(22,2))
		elseif f_type == 4
		then
			pkt.cols.protocol = "mOSPF Link State Update"

			local lsu = root:add(p_mospf_lsu, buf(16, buf:len()-16))
			lsu:add(f_lsu_seq, buf(16, 2))
			lsu:add(f_lsu_ttl, buf(18, 1))
			lsu:add(f_lsu_unused, buf(19, 1))
			lsu:add(f_lsu_nadv, buf(20, 4))
			local nadv = buf(20, 4):uint()

			for i = 0, nadv-1
			do 
				local lsa = root:add(p_mospf_lsa, buf(24+i*12, 12))
				lsa:add(f_lsa_network, buf(24+i*12,4))
				lsa:add(f_lsa_mask, buf(28+i*12,4))
				lsa:add(f_lsa_rid, buf(32+i*12,4))
			end
		end
	end

	local ip_encap_table = DissectorTable.get("ip.proto")
	ip_encap_table:add(90, p_mospf)
end
