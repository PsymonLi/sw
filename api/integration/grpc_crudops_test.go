package integration

import (
	"context"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/client"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/bookstore"
	"github.com/pensando/sw/venice/utils/kvstore"
	"github.com/pensando/sw/venice/utils/runtime"
	. "github.com/pensando/sw/venice/utils/testutils"
)

// validateObjectSpec Expects non-pointers in expected and result.
func validateObjectSpec(expected, result interface{}) bool {
	exp := reflect.Indirect(reflect.ValueOf(expected)).FieldByName("Spec").Interface()
	res := reflect.Indirect(reflect.ValueOf(result)).FieldByName("Spec").Interface()

	if !reflect.DeepEqual(exp, res) {
		tinfo.l.Infof("Values are %s[%+v] %s[%+v]", reflect.TypeOf(exp), exp, reflect.TypeOf(res), res)
		return false
	}
	return true
}

func addToWatchList(eventslist *[]kvstore.WatchEvent, obj interface{}, evtype kvstore.WatchEventType) []kvstore.WatchEvent {
	evp := reflect.ValueOf(obj).Interface()
	ev := kvstore.WatchEvent{
		Type:   evtype,
		Object: evp.(runtime.Object),
	}
	return append(*eventslist, ev)
}

func TestCrudOps(t *testing.T) {
	apiserverAddr := "localhost" + ":" + tinfo.apiserverport

	ctx := context.Background()
	// gRPC client
	apicl, err := client.NewGrpcUpstream("test", apiserverAddr, tinfo.l)
	if err != nil {
		t.Fatalf("cannot create grpc client")
	}

	// REST Client
	restcl, err := apiclient.NewRestAPIClient("http://localhost:" + tinfo.apigwport)
	if err != nil {
		t.Fatalf("cannot create REST client")
	}

	// Create some objects for use
	var pub, pub2 bookstore.Publisher
	{
		pub = bookstore.Publisher{
			ObjectMeta: api.ObjectMeta{
				Name: "Sahara",
			},
			TypeMeta: api.TypeMeta{
				Kind: "Publisher",
			},
			Spec: bookstore.PublisherSpec{
				Id:      "111",
				Address: "#1 hilane, timbuktoo",
				WebAddr: "http://sahara-books.org",
			},
		}
		pub2 = bookstore.Publisher{
			ObjectMeta: api.ObjectMeta{
				Name: "Kalahari",
			},
			TypeMeta: api.TypeMeta{
				Kind: "Publisher",
			},
			Spec: bookstore.PublisherSpec{
				Id:      "112",
				Address: "#2 lowside, timbuktoo",
				WebAddr: "http://rengtertopian.con:8080/",
			},
		}
	}

	// Setup Watcher and slices for validation.
	// For publisher
	var pRcvWatchEvents, pExpectWatchEvents []kvstore.WatchEvent
	// For orders
	var oRcvWatchEvents, oExpectWatchEvents []kvstore.WatchEvent
	// For Store
	var sRcvWatchEvents, sExpectWatchEvents []kvstore.WatchEvent
	var wg sync.WaitGroup
	wctx, cancel := context.WithCancel(ctx)
	waitWatch := make(chan bool)
	wg.Add(1)
	go func() {
		opts := api.ListWatchOptions{}
		watcher, err := apicl.BookstoreV1().Publisher().Watch(wctx, &opts)
		if err != nil {
			t.Fatalf("Failed to start watch (%s)\n", err)
		}
		orderWatcher, err := apicl.BookstoreV1().Order().Watch(wctx, &opts)
		if err != nil {
			t.Fatalf("Failed to start watch (%s)\n", err)
		}
		storeWatcher, err := apicl.BookstoreV1().Store().Watch(wctx, &opts)
		if err != nil {
			t.Fatalf("Failed to start watch (%s)\n", err)
		}

		close(waitWatch)
		active := true
		for active {
			select {
			case ev, ok := <-watcher.EventChan():
				t.Logf("ts[%s] received event [%v]", time.Now(), ok)
				if ok {
					t.Logf("  event [%+v]", *ev)
					pRcvWatchEvents = append(pRcvWatchEvents, *ev)
				} else {
					t.Logf("publisher watcher closed")
					active = false
				}
			case ev, ok := <-orderWatcher.EventChan():
				t.Logf("ts[%s] received event [%v]", time.Now(), ok)
				if ok {
					t.Logf("  event [%+v]", *ev)
					oRcvWatchEvents = append(oRcvWatchEvents, *ev)
				} else {
					t.Logf("order watcher closed")
					active = false
				}
			case ev, ok := <-storeWatcher.EventChan():
				t.Logf("ts[%s] received event [%v]", time.Now(), ok)
				if ok {
					t.Logf("  event [%+v]", *ev)
					sRcvWatchEvents = append(sRcvWatchEvents, *ev)
				} else {
					t.Logf("Store watcher closed")
					active = false
				}
			}
		}
		wg.Done()
	}()

	// Wait for watches to be established
	<-waitWatch

	// ========= TEST gRPC CRUD Operations ========= //
	t.Logf("test GRPC crud operations")
	{ // --- Create resource via gRPC --- //
		if ret, err := apicl.BookstoreV1().Publisher().Create(ctx, &pub); err != nil {
			t.Fatalf("failed to create publisher(%s)", err)
		} else {
			if !validateObjectSpec(&pub, ret) {
				t.Fatalf("updated object [Add] does not match \n\t[%+v]\n\t[%+v]", pub, ret)
			}
			evp := pub
			pExpectWatchEvents = addToWatchList(&pExpectWatchEvents, &evp, kvstore.Created)
		}
	}

	{ // --- Create  a second resource via gRPC ---//
		if ret, err := apicl.BookstoreV1().Publisher().Create(ctx, &pub2); err != nil {
			t.Fatalf("failed to create publisher(%s)", err)
		} else {
			if !validateObjectSpec(&pub2, ret) {
				t.Fatalf("updated object [Add] does not match \n\t[%+v]\n\t[%+v]", pub2, ret)
			}
			// Verify that the selflink in the objects
			evp := pub2
			pExpectWatchEvents = addToWatchList(&pExpectWatchEvents, &evp, kvstore.Created)
		}
	}

	{ // --- Get resource via gRPC ---//
		meta := api.ObjectMeta{Name: "Sahara"}
		ret, err := apicl.BookstoreV1().Publisher().Get(ctx, &meta)
		if err != nil {
			t.Fatalf("Error getting publisher (%s)", err)
		}

		t.Logf("Received object %+v", *ret)
		if !validateObjectSpec(&pub, ret) {
			t.Fatalf("updated object [Get] does not match \n\t[%+v]\n\t[%+v]", pub, ret)
		}
	}

	{ // --- List resources via gRPC ---//
		opts := api.ListWatchOptions{}
		pubs, err := apicl.BookstoreV1().Publisher().List(ctx, &opts)
		if err != nil {
			t.Fatalf("List failed %s", err)
		}
		if len(pubs) != 2 {
			t.Fatalf("expected [2] entries, got [%d]", len(pubs))
		}
	}

	{ // --- Duplicate add of the object via gRPC ---//
		if _, err := apicl.BookstoreV1().Publisher().Create(ctx, &pub); err == nil {
			t.Fatalf("Was able to create duplicate publisher")
		}
	}

	{ // --- Update resource via gRPC ---//
		pub.Spec.Address = "#22 hilane, timbuktoo"
		if ret, err := apicl.BookstoreV1().Publisher().Update(ctx, &pub); err != nil {
			t.Fatalf("failed to create publisher(%s)", err)
		} else {
			if !validateObjectSpec(&pub, ret) {
				t.Fatalf("updated object [Add] does not match \n\t[%+v]\n\t[%+v]", pub, ret)
			}
			evp := pub
			pExpectWatchEvents = addToWatchList(&pExpectWatchEvents, &evp, kvstore.Updated)
		}
	}

	{ // --- Delete Resource via gRPC --- //
		meta := api.ObjectMeta{Name: "Sahara"}
		ret, err := apicl.BookstoreV1().Publisher().Delete(ctx, &meta)
		if err != nil {
			t.Fatalf("failed to delete publisher(%s)", err)
		}
		if !reflect.DeepEqual(ret.Spec, pub.Spec) {
			t.Fatalf("Deleted object does not match \n\t[%+v]\n\t[%+v]", pub.Spec, ret.Spec)
		}
		evp := pub
		pExpectWatchEvents = addToWatchList(&pExpectWatchEvents, &evp, kvstore.Deleted)

	}

	// ========= Test REST CRUD Operations ========= //
	var order1, order2, order2mod bookstore.Order
	{
		order1 = bookstore.Order{
			ObjectMeta: api.ObjectMeta{
				Name: "test for pre-commit hook to generate new Order id - will be overwritten to order-<x> for POST",
			},
			TypeMeta: api.TypeMeta{
				Kind: "Order",
			},
			Spec: bookstore.OrderSpec{
				Id: "order-1",
				Order: []*bookstore.OrderItem{
					{
						ISBNId:   "XXXX",
						Quantity: 1,
					},
				},
			},
		}
		order2 = bookstore.Order{
			ObjectMeta: api.ObjectMeta{
				Name: "test for pre-commit hook to generate new Order id - will be overwritten to order-<x> for POST",
			},
			TypeMeta: api.TypeMeta{
				Kind: "Order",
			},
			Spec: bookstore.OrderSpec{
				Id: "order-2",
				Order: []*bookstore.OrderItem{
					{
						ISBNId:   "YYYY",
						Quantity: 3,
					},
				},
			},
		}
		// Create an new object for modified order 2 - cannot modify same object because watch validation needds original object.
		order2mod = bookstore.Order{
			ObjectMeta: api.ObjectMeta{
				Name: "order-2",
			},
			TypeMeta: api.TypeMeta{
				Kind: "Order",
			},
			Spec: bookstore.OrderSpec{
				Id: "order-2",
				Order: []*bookstore.OrderItem{
					{
						ISBNId:   "XXYY",
						Quantity: 30,
					},
				},
			},
		}

	}

	t.Logf("test REST crud operations")
	{ // ---  POST of the object via REST --- //
		retorder, err := restcl.BookstoreV1().Order().Create(ctx, &order1)
		if err != nil {
			t.Fatalf("Create of Order failed (%s)", err)
		}
		if !reflect.DeepEqual(retorder.Spec, order1.Spec) {
			t.Fatalf("Added Order object does not match \n\t[%+v]\n\t[%+v]", order1.Spec, retorder.Spec)
		}
		evp := order1
		oExpectWatchEvents = addToWatchList(&oExpectWatchEvents, &evp, kvstore.Created)
	}

	{ // ---  POST second  object via REST --- //
		retorder, err := restcl.BookstoreV1().Order().Create(ctx, &order2)
		if err != nil {
			t.Fatalf("Create of Order failed (%s)", err)
		}
		if !reflect.DeepEqual(retorder.Spec, order2.Spec) {
			t.Fatalf("Added Order object does not match \n\t[%+v]\n\t[%+v]", order1.Spec, retorder.Spec)
		}
		selflink := "/v1/bookstore/orders/" + retorder.Name
		if selflink != retorder.SelfLink {
			t.Errorf("Self link does not match expect [%s] got [%s]", selflink, retorder.SelfLink)
		}
		// The Status message should have been overwritten by the API gateway hooks
		if retorder.Status.Message != "Message filled by hook" {
			t.Errorf("API gateway post hook not called [%+v]", retorder)
		}
		evp := order2
		oExpectWatchEvents = addToWatchList(&oExpectWatchEvents, &evp, kvstore.Created)
	}

	{ // ---  POST an object that will be rejected by APIGw hooks --- //
		order3 := order2
		order3.Spec.Id = "order-reject"
		_, err := restcl.BookstoreV1().Order().Create(ctx, &order3)
		if err == nil {
			t.Fatalf("Create of Order should have failed")
		}
	}

	{ // ---  Get  object via REST --- //
		objectMeta := api.ObjectMeta{Name: "order-2"}
		retorder, err := restcl.BookstoreV1().Order().Get(ctx, &objectMeta)
		if err != nil {
			t.Fatalf("failed to get object Order via REST (%s)", err)
		}
		if !validateObjectSpec(retorder, order2) {
			t.Fatalf("updated object [Add] does not match \n\t[%+v]\n\t[%+v]", retorder, order2)
		}
	}

	{ // ---  LIST objects via REST --- //
		opts := api.ListWatchOptions{}
		retlist, err := restcl.BookstoreV1().Order().List(ctx, &opts)
		if err != nil {
			t.Fatalf("List operation returned error (%s)", err)
		}
		if len(retlist) != 2 {
			t.Fatalf("List expecting [2] elements got [%d]", len(retlist))
		}
	}

	{ // ---  PUT objects via REST --- //
		retorder, err := restcl.BookstoreV1().Order().Update(ctx, &order2mod)
		if err != nil {
			t.Fatalf("failed to update object Order via REST (%s)", err)
		}
		if !validateObjectSpec(retorder, order2mod) {
			t.Fatalf("updated object [Update] does not match \n\t[%+v]\n\t[%+v]", retorder, order2)
		}
		evp := order2mod
		oExpectWatchEvents = addToWatchList(&oExpectWatchEvents, &evp, kvstore.Updated)
	}

	{ // ---  DELETE objects via REST --- //
		objectMeta := api.ObjectMeta{Name: "order-1"}
		retorder, err := restcl.BookstoreV1().Order().Delete(ctx, &objectMeta)
		if err != nil {
			t.Fatalf("failed to delete object Order via REST (%s)", err)
		}
		if !validateObjectSpec(retorder, order1) {
			t.Fatalf("updated object [Delete] does not match \n\t[%+v]\n\t[%+v]", retorder, order1)
		}
		oExpectWatchEvents = addToWatchList(&oExpectWatchEvents, &order1, kvstore.Deleted)
	}

	// ========= Test Validation and Status update Operations ========= //
	var book1, book1mod bookstore.Book
	{
		book1 = bookstore.Book{
			ObjectMeta: api.ObjectMeta{
				Name: "book1",
			},
			TypeMeta: api.TypeMeta{
				Kind: "book",
			},
			Spec: bookstore.BookSpec{
				ISBNId:   "111-2-31-123456-0",
				Author:   "foo",
				Category: "JunkValue",
			},
		}
		book1mod = bookstore.Book{
			ObjectMeta: api.ObjectMeta{
				Name: "book1",
			},
			TypeMeta: api.TypeMeta{
				Kind: "book",
			},
			Spec: bookstore.BookSpec{
				ISBNId:   "111-2-31-123456-0",
				Author:   "bar",
				Category: "JunkValue2",
			},
			Status: bookstore.BookStatus{
				Inventory: 10,
			},
		}
	}
	{ // Create a new Book entry
		_, err := apicl.BookstoreV1().Book().Create(ctx, &book1)
		if err == nil {
			t.Fatalf("Book create expected to fail due to validation")
		}
		book1.Spec.Category = "ChildrensLit"
		retbook, err := apicl.BookstoreV1().Book().Create(ctx, &book1)
		if err != nil {
			t.Fatalf("Book create failed [%s]", err)
		}
		if !reflect.DeepEqual(retbook.Spec, book1.Spec) {
			t.Fatalf("Added Order object does not match \n\t[%+v]\n\t[%+v]", book1.Spec, retbook.Spec)
		}
	}
	{ // Update the Book with status via gRPC with Validation
		_, err := apicl.BookstoreV1().Book().Update(ctx, &book1mod)
		if err == nil {
			t.Fatalf("Validation expected to fail")
		}
		book1mod.Spec.Category = "ChildrensLit"
		_, err = apicl.BookstoreV1().Book().Update(ctx, &book1mod)
		if err != nil {
			t.Fatalf("Expected to succeed")
		}
	}
	{ // Update the Book with Status via REST
		book1mod.Status.Inventory = 100
		_, err = restcl.BookstoreV1().Book().Update(ctx, &book1mod)
		if err != nil {
			t.Fatalf("Failed to update book via REST (%s)", err)
		}
		objectMeta := api.ObjectMeta{Name: "book1"}
		t.Logf("requesting Get for object %+v", objectMeta)
		retbook, err := restcl.BookstoreV1().Book().Get(ctx, &objectMeta)
		if err != nil {
			t.Fatalf("Failed to retrieve book via REST (%s)", err)
		}
		if retbook.Status.Inventory != 10 {
			t.Fatalf("REST update was able to update status fields")
		}
	}

	// ===== Test Operations on Singleton Object ===== //
	{ // Create via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store"
		_, err := apicl.BookstoreV1().Store().Create(ctx, &storeObj)
		if err != nil {
			t.Fatalf("gRPC create of singleton failed (%s)", err)
		}
		evp := storeObj
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, &evp, kvstore.Created)
	}

	{ // Create Duplicate via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store2"
		_, err := apicl.BookstoreV1().Store().Create(ctx, &storeObj)
		if err == nil {
			t.Fatalf("gRPC create of duplicate singleton succeded")
		}
	}

	{ // Get via the gRPC
		objectMeta := api.ObjectMeta{Name: "Dummy Store"}
		ret, err := apicl.BookstoreV1().Store().Get(ctx, &objectMeta)
		if err != nil {
			t.Fatalf("gRPC Get of singleton failed")
		}
		if ret.Spec.Contact != "Test Store" {
			t.Fatalf("Singleton object does notmatch exp[Test Store] got [%s]", ret.Spec.Contact)
		}
	}
	{ // Create via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store2"
		_, err := apicl.BookstoreV1().Store().Update(ctx, &storeObj)
		if err != nil {
			t.Fatalf("gRPC create of singleton failed (%s)", err)
		}
		evp := storeObj
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, &evp, kvstore.Updated)
	}
	{ // List via the gRPC
		opts := api.ListWatchOptions{}
		ret, err := apicl.BookstoreV1().Store().List(ctx, &opts)
		if err != nil {
			t.Fatalf("gRPC list of singleton failed ")
		}
		if len(ret) != 1 {
			t.Fatalf("Singleton object list got more than one item [%v] ", ret)
		}
		if ret[0].Spec.Contact != "Test Store2" {
			t.Fatalf("Singleton object does notmatch exp[Test Store] got [%s]", ret[0].Spec.Contact)
		}
	}
	{ // Delete via gRPC
		meta := api.ObjectMeta{Name: "Dummy"}
		storeObj, err := apicl.BookstoreV1().Store().Delete(ctx, &meta)
		if err != nil {
			t.Fatalf("Failed to delete singleton object (%s)", err)
		}
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, storeObj, kvstore.Deleted)
		opts := api.ListWatchOptions{}
		ret, err := apicl.BookstoreV1().Store().List(ctx, &opts)
		if err != nil {
			t.Fatalf("gRPC List of singleton failed")
		}
		if len(ret) != 0 {
			t.Fatalf("Singleton object list got more than zero item [%v] ", ret)
		}
	}

	// ===== Test Operations on Singleton object via REST ===== //
	{ // Create via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store"
		_, err := restcl.BookstoreV1().Store().Create(ctx, &storeObj)
		if err != nil {
			t.Fatalf("REST create of singleton failed (%s)", err)
		}
		evp := storeObj
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, &evp, kvstore.Created)
	}

	{ // Create Duplicate via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store2"
		_, err := restcl.BookstoreV1().Store().Create(ctx, &storeObj)
		if err == nil {
			t.Fatalf("REST create of duplicate singleton succeded")
		}
	}

	{ // Get via the gRPC
		objectMeta := api.ObjectMeta{Name: "Dummy Store"}
		ret, err := restcl.BookstoreV1().Store().Get(ctx, &objectMeta)
		if err != nil {
			t.Fatalf("REST Get of singleton failed")
		}
		if ret.Spec.Contact != "Test Store" {
			t.Fatalf("Singleton object does notmatch exp[Test Store] got [%s]", ret.Spec.Contact)
		}
	}
	{ // Create via the gRPC
		storeObj := bookstore.Store{}
		storeObj.Spec.Contact = "Test Store2"
		_, err := restcl.BookstoreV1().Store().Update(ctx, &storeObj)
		if err != nil {
			t.Fatalf("REST create of singleton failed (%s)", err)
		}
		evp := storeObj
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, &evp, kvstore.Updated)
	}
	{ // Delete via gRPC
		meta := api.ObjectMeta{Name: "Dummy"}
		storeObj, err := restcl.BookstoreV1().Store().Delete(ctx, &meta)
		if err != nil {
			t.Fatalf("Failed to delete singleton object (%s)", err)
		}
		sExpectWatchEvents = addToWatchList(&sExpectWatchEvents, storeObj, kvstore.Deleted)
		objectMeta := api.ObjectMeta{Name: "Test Strore2"}
		_, err = restcl.BookstoreV1().Store().Get(ctx, &objectMeta)
		if err == nil {
			t.Fatalf("REST get succeeded after Delete")
		}
	}

	// ===== Validate Watch Events received === //
	AssertEventually(t,
		func() (bool, interface{}) { return len(pExpectWatchEvents) == len(pRcvWatchEvents), nil },
		"failed to receive all watch events",
		"10ms",
		"9s")
	AssertEventually(t,
		func() (bool, interface{}) { return len(oExpectWatchEvents) == len(oRcvWatchEvents), nil },
		"failed to receive all watch events",
		"10ms",
		"9s")
	AssertEventually(t,
		func() (bool, interface{}) { return len(sExpectWatchEvents) == len(sRcvWatchEvents), nil },
		"failed to receive all watch events",
		"10ms",
		"9s")
	cancel()
	wg.Wait()

	for k := range pExpectWatchEvents {
		if pExpectWatchEvents[k].Type != pRcvWatchEvents[k].Type {
			t.Fatalf("mismatched event type expected (%s) got (%s)", pExpectWatchEvents[k].Type, pRcvWatchEvents[k].Type)
		}
		if !validateObjectSpec(pExpectWatchEvents[k].Object, pRcvWatchEvents[k].Object) {
			t.Fatalf("watch event object [%s] does not match \n\t[%+v]\n\t[%+v]", pExpectWatchEvents[k].Type, pExpectWatchEvents[k].Object, pRcvWatchEvents[k].Object)
		}
	}
	for k := range oExpectWatchEvents {
		if oExpectWatchEvents[k].Type != oRcvWatchEvents[k].Type {
			t.Fatalf("mismatched event type expected (%s) got (%s)", oExpectWatchEvents[k].Type, oRcvWatchEvents[k].Type)
		}
		if !validateObjectSpec(oExpectWatchEvents[k].Object, oRcvWatchEvents[k].Object) {
			t.Fatalf("watch event object [%s] does not match \n\t[%+v]\n\t[%+v]", oExpectWatchEvents[k].Type, oExpectWatchEvents[k].Object, oRcvWatchEvents[k].Object)
		}
	}

	for k := range sExpectWatchEvents {
		if sExpectWatchEvents[k].Type != sRcvWatchEvents[k].Type {
			t.Fatalf("mismatched event type expected (%s) got (%s)", sExpectWatchEvents[k].Type, sRcvWatchEvents[k].Type)
		}
		if !validateObjectSpec(sExpectWatchEvents[k].Object, sRcvWatchEvents[k].Object) {
			t.Fatalf("watch event object [%s] does not match \n\t[%+v]\n\t[%+v]", sExpectWatchEvents[k].Type, sExpectWatchEvents[k].Object, sRcvWatchEvents[k].Object)
		}
	}

	// Test Action Functions - gRPC
	actreq := bookstore.ApplyDiscountReq{}
	actreq.Name = "order-2"
	ret, err := apicl.BookstoreV1().Order().Applydiscount(ctx, &actreq)
	if err != nil {
		t.Errorf("gRPC: apply discount action failed (%s)", err)
	}
	if ret.Status.Status != "DISCOUNTED" {
		t.Errorf("gRPC: hook did not run")
	}

	// Reset the object:
	ret, err = apicl.BookstoreV1().Order().Cleardiscount(ctx, &actreq)
	if err != nil {
		t.Errorf("gRPC: clear discount action failed (%s)", err)
	}
	if ret.Status.Status != "PROCESSING" {
		t.Errorf("gRPC: hook did not run")
	}

	// Restock action on Collection
	restockReq := bookstore.RestockRequest{}
	restkresp, err := apicl.BookstoreV1().Book().Restock(ctx, &restockReq)
	if err != nil {
		t.Errorf("gRPC: Restock action failed (%s)", err)
	}
	if restkresp.Count != 3 {
		t.Errorf("gRPC: restock action did not go through hook")
	}

	// Action on singleton object
	outReq := bookstore.OutageRequest{}
	outresp, err := apicl.BookstoreV1().Store().AddOutage(ctx, &outReq)
	if err != nil {
		t.Errorf("gRPC: Outage action failed (%s)", err)
	}
	if len(outresp.Status.CurrentOutages) != 1 {
		t.Errorf("gRPC: Outage action did not go through hook")
	}

	// Test Action Functions - REST
	ret, err = restcl.BookstoreV1().Order().Applydiscount(ctx, &actreq)
	if err != nil {
		t.Errorf("REST: apply discount action failed (%s)", err)
	}
	if ret.Status.Status != "DISCOUNTED" {
		t.Errorf("REST: hook did not run")
	}

	// Restock action on Collection
	restkresp, err = restcl.BookstoreV1().Book().Restock(ctx, &restockReq)
	if err != nil {
		t.Errorf("REST: Restock action failed (%s)", err)
	}
	if restkresp.Count != 3 {
		t.Errorf("REST: restock action did not go through hook")
	}

	// Action on singleton object
	outresp, err = restcl.BookstoreV1().Store().AddOutage(ctx, &outReq)
	if err != nil {
		t.Errorf("REST: Outage action failed (%s)", err)
	}
	if len(outresp.Status.CurrentOutages) != 1 {
		t.Errorf("REST: Outage action did not go through hook")
	}

}
