pipeline {
  agent any
  stages {
    stage('Build, Test, and Package') {
      parallel {        
        
        
        stage('SGX1 Package RelWithDebInfo') {
          agent {
            node {
              label 'hardware'
          }

          }
          steps {
            sh 'bash ./scripts/test-build-config -p SGX1 -b RelWithDebInfo --build_package'
            sh 'bash ./scripts/deploy-docs build ssh'
          }
        }
      }
    }
  }
}
