pipeline {
  agent any
  stages {
    stage('Build, Test, and Package') {
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
          }
        }
        stage('SGX1FLC Package Release') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1FLC -b Release -d --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/Release/SGX1FLC/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/Release/SGX1FLC/', containerName: 'oejenkins')
          }
        }
        stage('SGX1FLC Package RelWithDebInfo') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1FLC -b RelWithDebInfo -d --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/RelWithDebInfo/SGX1FLC/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/RelWithDebInfo/SGX1FLC/', containerName: 'oejenkins')
          }
        }
        stage('SGX1 Package Debug') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1 -b Debug --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/Debug/SGX1/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/Debug/SGX1/', containerName: 'oejenkins')
          }
        }
        stage('SGX1 Package Release') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1 -b Release --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/Release/SGX1/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/Release/SGX1/', containerName: 'oejenkins')
          }
        }
        stage('SGX1 Package RelWithDebInfo') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1 -b RelWithDebInfo --build_package'
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/${BUILD_NUMBER}/RelWithDebInfo/SGX1/', containerName: 'oejenkins')
            azureUpload(storageCredentialId: 'oejenkinsciartifacts_storageaccount', filesPath: 'build/*.deb', storageType: 'blobstorage', virtualPath: 'master/latest/RelWithDebInfo/SGX1/', containerName: 'oejenkins')
          }
        }
       stage('deploy-docs') {
         agent{
            node {
              label 'hardware'
          }
         }
         steps {
          withCredentials([usernamePassword(credentialsId: '40060061-6050-40f7-ac6a-53aeb767245f', passwordVariable: 'SERVICE_PRINCIPAL_PASSWORD', usernameVariable: 'SERVICE_PRINCIPAL_ID')]) {
            sh 'bash ./scripts/deploy-docs build ssh'
          }
        }
      }
    }
  }
}
