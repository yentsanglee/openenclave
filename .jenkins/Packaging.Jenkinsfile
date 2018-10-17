pipeline {
  agent any
  stages {
      parallel {
        stage('SGX1FLC Package Debug') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1FLC -b Debug -d --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/Debug/SGX1FLC/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/Debug/SGX1FLC/', containerName: 'oejenkins')
             
          withCredentials([usernamePassword(credentialsId: '40060061-6050-40f7-ac6a-53aeb767245f', passwordVariable: 'SERVICE_PRINCIPAL_PASSWORD', usernameVariable: 'SERVICE_PRINCIPAL_ID')]) {
            sh 'bash ./scripts/deploy-docs build ssh'
          }   
        
        }
      }
    }
  }
}
