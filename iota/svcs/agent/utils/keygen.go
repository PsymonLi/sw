package utils

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"io/ioutil"
	"log"
	"os"
	"os/user"

	"golang.org/x/crypto/ssh"
)

//SSHKeygen generate SSH key gen
func SSHKeygen() {

	sshDir := os.Getenv("HOME") + "/.ssh"
	os.Mkdir(sshDir, 0755)
	savePrivateFileTo := sshDir + "/id_rsa"
	savePublicFileTo := sshDir + "/id_rsa.pub"
	bitSize := 4096

	//Don't generate the key if it already started.
	_, err1 := os.Stat(savePrivateFileTo)
	_, err2 := os.Stat(savePublicFileTo)
	if !os.IsNotExist(err1) && !os.IsNotExist(err2) {
		return
	}

	privateKey, err := generatePrivateKey(bitSize)
	if err != nil {
		log.Fatal(err.Error())
	}

	publicKeyBytes, err := generatePublicKey(&privateKey.PublicKey)
	if err != nil {
		log.Fatal(err.Error())
	}

	privateKeyBytes := encodePrivateKeyToPEM(privateKey)

	err = writeKeyToFile(privateKeyBytes, savePrivateFileTo, 0600)
	if err != nil {
		log.Fatal(err.Error())
	}

	err = writeKeyToFile([]byte(publicKeyBytes), savePublicFileTo, 0644)
	if err != nil {
		log.Fatal(err.Error())
	}

	// Copy the generated key file to 'root' as well
	currentUser, err := user.Current()
	if err != nil {
		log.Println("Could not get currentUser")
		return
	}

	if os.Getenv("HOME") != currentUser.HomeDir {
		sshDir = currentUser.HomeDir + "/.ssh"
		os.Mkdir(sshDir, 0755)
		savePrivateFileTo = sshDir + "/id_rsa"
		savePublicFileTo = sshDir + "/id_rsa.pub"

		err = writeKeyToFile(privateKeyBytes, savePrivateFileTo, 0600)
		if err != nil {
			log.Fatal(err.Error())
		}

		err = writeKeyToFile([]byte(publicKeyBytes), savePublicFileTo, 0644)
		if err != nil {
			log.Fatal(err.Error())
		}
	}

}

// generatePrivateKey creates a RSA Private Key of specified byte size
func generatePrivateKey(bitSize int) (*rsa.PrivateKey, error) {
	// Private Key generation
	privateKey, err := rsa.GenerateKey(rand.Reader, bitSize)
	if err != nil {
		return nil, err
	}

	// Validate Private Key
	err = privateKey.Validate()
	if err != nil {
		return nil, err
	}

	log.Println("Private Key generated")
	return privateKey, nil
}

// encodePrivateKeyToPEM encodes Private Key from RSA to PEM format
func encodePrivateKeyToPEM(privateKey *rsa.PrivateKey) []byte {
	// Get ASN.1 DER format
	privDER := x509.MarshalPKCS1PrivateKey(privateKey)

	// pem.Block
	privBlock := pem.Block{
		Type:    "RSA PRIVATE KEY",
		Headers: nil,
		Bytes:   privDER,
	}

	// Private key in PEM format
	privatePEM := pem.EncodeToMemory(&privBlock)

	return privatePEM
}

// generatePublicKey take a rsa.PublicKey and return bytes suitable for writing to .pub file
// returns in the format "ssh-rsa ..."
func generatePublicKey(privatekey *rsa.PublicKey) ([]byte, error) {
	publicRsaKey, err := ssh.NewPublicKey(privatekey)
	if err != nil {
		return nil, err
	}

	pubKeyBytes := ssh.MarshalAuthorizedKey(publicRsaKey)

	log.Println("Public key generated")
	return pubKeyBytes, nil
}

// writePemToFile writes keys to a file
func writeKeyToFile(keyBytes []byte, saveFileTo string, perm os.FileMode) error {
	err := ioutil.WriteFile(saveFileTo, keyBytes, perm)
	if err != nil {
		return err
	}

	log.Printf("Key saved to: %s", saveFileTo)
	return nil
}
