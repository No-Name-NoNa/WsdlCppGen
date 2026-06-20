package main

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestAddServiceHandler(t *testing.T) {
	body := `<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:tns="http://example.com/addservice">
  <soap:Body>
    <tns:AddRequest>
      <num1>3</num1>
      <num2>5</num2>
    </tns:AddRequest>
  </soap:Body>
</soap:Envelope>`

	request := httptest.NewRequest(http.MethodPost, "/services/AddService", strings.NewReader(body))
	response := httptest.NewRecorder()

	addServiceHandler(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("unexpected status: %d", response.Code)
	}
	if !strings.Contains(response.Body.String(), "<result>8</result>") {
		t.Fatalf("response does not contain result 8: %s", response.Body.String())
	}
}

func TestWsdlEndpoint(t *testing.T) {
	request := httptest.NewRequest(http.MethodGet, "/services/AddService?wsdl", nil)
	response := httptest.NewRecorder()

	addServiceHandler(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("unexpected status: %d", response.Code)
	}
	if !strings.Contains(response.Body.String(), "AddService") {
		t.Fatalf("WSDL response does not contain AddService")
	}
}
