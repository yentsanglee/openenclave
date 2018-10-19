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
withCredentials([usernamePassword(credentialsId: 'https_gh_pages_push', passwordVariable: 'SERVICE_PRINCIPAL_PASSWORD', usernameVariable: 'SERVICE_PRINCIPAL_ID')]) {
            sh 'bash ./scripts/deploy-docs build https $SERVICE_PRINCIPAL_ID $SERVICE_PRINCIPAL_PASSWORD'
        }
      }
    }
  }
}
