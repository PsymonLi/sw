// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

package utils

import (
	"testing"
)

func TestConvertMAC(t *testing.T) {
	mac := "00:15:CF:80:F8:13"
	dottedMAC := "0015.CF80.F813"
	if dottedMAC != ConvertMAC(mac) {
		t.Errorf("MAC conversion failed, Expected %s got %s", dottedMAC, ConvertMAC(mac))
	}
}

func TestUint64ToMac(t *testing.T) {
	cases := []struct {
		in  uint64
		out string
	}{
		{
			in:  0,
			out: "0000.0000.0000",
		},
		{
			in:  1,
			out: "0000.0000.0001",
		},
		{
			in:  0xdeadbeef,
			out: "0000.dead.beef",
		},
	}

	for _, c := range cases {
		if c.out != Uint64ToMac(c.in) {
			t.Errorf("failed [%v] -> [%v]", c.in, Uint64ToMac(c.in))
		}
	}
}
