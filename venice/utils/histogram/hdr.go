package histogram

import (
	"expvar"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	hdr "github.com/codahale/hdrhistogram"
)

// HistMap is a container for the histograms
type HistMap struct {
	histmap   sync.Map
	histcount sync.Map
}

type safeHistogram struct {
	sync.Mutex
	*hdr.Histogram
}

// Record records an measurement in the <name> histogram.
func (m *HistMap) Record(name string, d time.Duration) {
	if h, ok := m.histmap.Load(name); !ok {
		h = hdr.New(0, 100000000000, 4)
		sh := &safeHistogram{Histogram: h.(*hdr.Histogram)}
		sh.RecordValue(d.Nanoseconds())
		m.histmap.Store(name, sh)
		var cnt uint64 = 1
		m.histcount.Store(name, &cnt)
	} else {
		if c, ok := m.histcount.Load(name); ok {
			cnt := c.(*uint64)
			atomic.AddUint64(cnt, 1)
		}
		sh := h.(*safeHistogram)
		sh.Lock()
		sh.RecordValue(d.Nanoseconds())
		sh.Unlock()
	}
}

func (m *HistMap) sprintone(key string, h *safeHistogram) string {
	c, _ := m.histcount.Load(key)
	cnt := c.(*uint64)
	ret := fmt.Sprintf("--[ %s ] Total: %d/%d Mean: %.3f Max: %.3f Min: %.3f\n", key, h.TotalCount(), atomic.LoadUint64(cnt),
		float64(h.Mean())/float64(time.Millisecond),
		float64(h.Max())/float64(time.Millisecond),
		float64(h.Min())/float64(time.Millisecond))
	ret = ret + fmt.Sprintf("     10: %.3f 50: %.3f 90: %.3f 99: %.3f\n",
		float64(h.ValueAtQuantile(10))/float64(time.Millisecond),
		float64(h.ValueAtQuantile(50))/float64(time.Millisecond),
		float64(h.ValueAtQuantile(90))/float64(time.Millisecond),
		float64(h.ValueAtQuantile(99))/float64(time.Millisecond))
	return ret
}

// PrintAll prints all the histograms
func (m *HistMap) PrintAll() {
	printhdr := func(key, value interface{}) bool {
		h := value.(*safeHistogram)
		h.Lock()
		ret := m.sprintone(key.(string), h)
		fmt.Printf("%s", ret)
		h.Unlock()
		return true
	}
	m.histmap.Range(printhdr)
}

// PrintOne prints one histogram
func (m *HistMap) PrintOne(key string) {
	if value, ok := m.histmap.Load(key); ok {
		h := value.(*safeHistogram)
		h.Lock()
		ret := m.sprintone(key, h)
		fmt.Printf("%s", ret)
		h.Unlock()
	}
}

// SprintOne returns one histogram as string
func (m *HistMap) SprintOne(key string) string {
	ret := ""
	if value, ok := m.histmap.Load(key); ok {
		h := value.(*safeHistogram)
		h.Lock()
		ret = m.sprintone(key, h)
		h.Unlock()

	}
	return ret
}

// GetStats returns a map of all collected Stats in human friendly form
func (m *HistMap) GetStats() map[string]Stats {
	ret := make(map[string]Stats)
	fn := func(key, value interface{}) bool {
		h := value.(*safeHistogram)
		k := key.(string)
		h.Lock()
		stat := Stats{
			Count:       h.TotalCount(),
			MinMs:       float64(h.Min()) / float64(time.Millisecond),
			MaxMs:       float64(h.Max()) / float64(time.Millisecond),
			MeanMs:      float64(h.Mean()) / float64(time.Millisecond),
			StdDev:      h.StdDev(),
			Perctl10:    float64(h.ValueAtQuantile(10)) / float64(time.Millisecond),
			Perctl50:    float64(h.ValueAtQuantile(50)) / float64(time.Millisecond),
			Perctl75:    float64(h.ValueAtQuantile(75)) / float64(time.Millisecond),
			Perctl90:    float64(h.ValueAtQuantile(90)) / float64(time.Millisecond),
			Perctl99:    float64(h.ValueAtQuantile(99)) / float64(time.Millisecond),
			Perctl99_99: float64(h.ValueAtQuantile(99.99)) / float64(time.Millisecond),
		}
		h.Unlock()
		ret[k] = stat
		return true
	}
	m.histmap.Range(fn)
	return ret
}

var defHistMap HistMap

// Record records an measurement in the <name> histogram
func Record(name string, d time.Duration) {
	defHistMap.Record(name, d)
}

// PrintAll prints all the hdrs in default histmap
func PrintAll() {
	defHistMap.PrintAll()
}

// PrintOne prints one histogram in default histmap
func PrintOne(key string) {
	defHistMap.PrintOne(key)
}

// GetStats returns a map of all collected Stats in human friendly form
func GetStats() map[string]Stats {
	return defHistMap.GetStats()
}

// Stats shows the histogram in human friendly format
type Stats struct {
	Count                                                         int64
	MinMs, MaxMs, MeanMs, StdDev                                  float64
	Perctl10, Perctl50, Perctl75, Perctl90, Perctl99, Perctl99_99 float64
}

func init() {
	var histfunc expvar.Func
	histfunc = func() interface{} {
		return GetStats()
	}
	expvar.Publish("histograms", histfunc)
}
